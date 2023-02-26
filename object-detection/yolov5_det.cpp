#include <iostream>
#include <chrono>
#include <cmath>
#include "cuda_utils.h"
#include "logging.h"
#include "common.h"
#include "utils.h"

// === ZED ===
#include <sl/Camera.hpp>
// === ZED ===

// === connection ===
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
// === connection ===

#define DEVICE 0  // GPU id
#define NMS_THRESH 0.4
#define CONF_THRESH 0.7
#define BATCH_SIZE 1

// stuff we know about the network and the input/output blobs
static const int INPUT_H = Yolo::INPUT_H;
static const int INPUT_W = Yolo::INPUT_W;
static const int CLASS_NUM = Yolo::CLASS_NUM;
static const int OUTPUT_SIZE = Yolo::MAX_OUTPUT_BBOX_COUNT * sizeof(Yolo::Detection) / sizeof(float) + 1;  // we assume the yololayer outputs no more than MAX_OUTPUT_BBOX_COUNT boxes that conf >= 0.1
const char* INPUT_BLOB_NAME = "data";
const char* OUTPUT_BLOB_NAME = "prob";
static Logger gLogger;

// TCP client
const char* ip = "127.0.0.1";
uint16_t port = 54000;
int sock = 0;
struct sockaddr_in serverAddress;
int serverAddressSize = sizeof(serverAddress);

static int get_width(int x, float gw, int divisor = 8) {
    return int(ceil((x * gw) / divisor)) * divisor;
}

static int get_depth(int x, float gd) {
    if (x == 1) return 1;
    int r = round(x * gd);
    if (x * gd - int(x * gd) == 0.5 && (int(x * gd) % 2) == 0) {
        --r;
    }
    return std::max<int>(r, 1);
}

void doInference(IExecutionContext& context, cudaStream_t& stream, void **buffers, float* input, float* output, int batchSize) {
    // infer on the batch asynchronously, and DMA output back to host
    CUDA_CHECK(cudaMemcpyAsync(buffers[0], input, batchSize * 3 * INPUT_H * INPUT_W * sizeof (float), cudaMemcpyHostToDevice, stream));
    context.enqueue(batchSize, buffers, stream, nullptr);
    CUDA_CHECK(cudaMemcpyAsync(output, buffers[1], batchSize * OUTPUT_SIZE * sizeof(float), cudaMemcpyDeviceToHost, stream));
    cudaStreamSynchronize(stream);
}

bool parse_args(int argc, char** argv, std::string& engine) {
    if (argc < 2)
        return false;
    if (argc == 2)
        engine = std::string(argv[1]);
    else
        return false;
    return true;
}

bool tcpClientSetup(){
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std::cerr << "Error during socket creation" << std::endl;
        return false;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    bzero((char *) &serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ip);
    serverAddress.sin_port = htons(port);

    if(connect(sock, (struct sockaddr*)&serverAddress, serverAddressSize) < 0){
        if(errno == EINPROGRESS)
            std::cout << "EINPROGRESS" << std::endl;
        else {
            perror("Connection error: ");
            return false;
        }
    }    
    return true;
}

bool tcpSendDetection(int id, int label, sl::float3 position, std::vector<sl::float3> bbox){
    std::string msg = "";
    std::cout << "ID: " + std::to_string(id) << std::endl;
    std::cout << "label: " + std::to_string(label) << std::endl;
    msg.append(std::to_string(id) + ";");
    msg.append(std::to_string(label) + ";");
    msg.append(
        std::to_string(position[0]) + "," +
        std::to_string(position[1]) + "," +
        std::to_string(position[2]) + ";"
    );
    std::cout << 
        "(" + std::to_string(position[0]) + "," +
        std::to_string(position[1]) + "," +
        std::to_string(position[2]) + ")"
    << std::endl;
    for(int i=0; i<bbox.size(); ++i){
        msg.append(
            std::to_string(bbox[i][0]) + "," +
            std::to_string(bbox[i][1]) + "," +
            std::to_string(bbox[i][2])
        );
        std::cout << 
            std::to_string(bbox[i][0]) + "," +
            std::to_string(bbox[i][1]) + "," +
            std::to_string(bbox[i][2])
        << std::endl;
        if(i<bbox.size()-1)
            msg.append(";");
    }
    // std::cout << "msg: " + msg << std::endl;
    if(send(sock, msg.c_str(), strlen(msg.c_str()), 0) < 0){
        std::cout << "SEND ERROR, errno=" + std::to_string(errno) << std::endl;
        return false;
    }
    return true;
}

void tcpDispatch(std::vector<sl::ObjectData> &objs, std::vector<int> labels){
    std::cout << "<begin frame>" << std::endl;
    for(int i=0; i < objs.size(); i++) {    // for each object detected in frame
        // tcpSendDetection(labels[i], objs[i].position, objs[i].bounding_box);
        tcpSendDetection(objs[i].id, objs[i].raw_label, objs[i].position, objs[i].bounding_box);
    }
    std::cout << "<end frame>" << std::endl;
}

// === ZED ===
void print(std::string msg_prefix, sl::ERROR_CODE err_code, std::string msg_suffix) {
    std::cout << "[Sample] ";
    if (err_code != sl::ERROR_CODE::SUCCESS)
        std::cout << "[Error] ";
    std::cout << msg_prefix << " ";
    if (err_code != sl::ERROR_CODE::SUCCESS) {
        std::cout << " | " << toString(err_code) << " : ";
        std::cout << toVerbose(err_code);
    }
    if (!msg_suffix.empty())
        std::cout << " " << msg_suffix;
    std::cout << std::endl;
}

std::vector<sl::uint2> cvt(const cv::Rect &bbox_in){
    std::vector<sl::uint2> bbox_out(4);
    bbox_out[0] = sl::uint2(bbox_in.x, bbox_in.y);
    bbox_out[1] = sl::uint2(bbox_in.x + bbox_in.width, bbox_in.y);
    bbox_out[2] = sl::uint2(bbox_in.x + bbox_in.width, bbox_in.y + bbox_in.height);
    bbox_out[3] = sl::uint2(bbox_in.x, bbox_in.y + bbox_in.height);
    return bbox_out;
}
// === ZED ===

int main(int argc, char** argv) {
    std::string engine_name = "";
    if (!parse_args(argc, argv, engine_name)) {
        std::cerr << "arguments not right!" << std::endl;
        std::cerr << "./yolov5_det [.engine]  // deserialize plan file and run inference" << std::endl;
        return -1;
    }

    if(!tcpClientSetup()){
        std::cerr << "Error during TCP client setup." << std::endl;
        return -1;
    }

    // === ZED ===
    // Opening the ZED camera before the model deserialization to avoid cuda context issue
    sl::Camera zed;
    sl::InitParameters init_parameters;
    sl::RuntimeParameters runtime_parameters;
    init_parameters.camera_resolution = sl::RESOLUTION::HD720;
    init_parameters.camera_fps = 15;
    init_parameters.sdk_verbose = true;
    init_parameters.coordinate_units = sl::UNIT::CENTIMETER;
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL;
    //init_parameters.depth_minimum_distance = 10; //0.1;
    //init_parameters.depth_maximum_distance = 300; //2.0;
    init_parameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Z_UP; // OpenGL's coordinate system is right_handed
    runtime_parameters.sensing_mode = sl::SENSING_MODE::FILL;
    if (argc > 3) {
        std::string zed_opt = argv[3];
        if (zed_opt.find(".svo") != std::string::npos)
            init_parameters.input.setFromSVOFile(zed_opt.c_str());
    }
    // Open the camera
    auto returned_state = zed.open(init_parameters);
    if (returned_state != sl::ERROR_CODE::SUCCESS) {
        print("Camera Open", returned_state, "Exit program.");
        return EXIT_FAILURE;
    }
    zed.enablePositionalTracking();
    // Custom OD
    sl::ObjectDetectionParameters detection_parameters;
    // detection_parameters.image_sync = false;
    detection_parameters.enable_tracking = true;
    detection_parameters.enable_mask_output = false; // designed to give person pixel mask
    detection_parameters.detection_model = sl::DETECTION_MODEL::CUSTOM_BOX_OBJECTS;
    returned_state = zed.enableObjectDetection(detection_parameters);
    if (returned_state != sl::ERROR_CODE::SUCCESS) {
        print("enableObjectDetection", returned_state, "\nExit program.");
        zed.close();
        return EXIT_FAILURE;
    }
    auto camera_config = zed.getCameraInformation().camera_configuration;
    sl::Resolution pc_resolution(std::min((int) camera_config.resolution.width, 720), std::min((int) camera_config.resolution.height, 404));
    auto camera_info = zed.getCameraInformation(pc_resolution).camera_configuration;
    // === ZED ===

    // deserialize the .engine and run inference
    std::ifstream file(engine_name, std::ios::binary);
    if (!file.good()) {
        std::cerr << "read " << engine_name << " error!" << std::endl;
        return -1;
    }
    char *trtModelStream = nullptr;
    size_t size = 0;
    file.seekg(0, file.end);
    size = file.tellg();
    file.seekg(0, file.beg);
    trtModelStream = new char[size];
    assert(trtModelStream);
    file.read(trtModelStream, size);
    file.close();

    static float data[BATCH_SIZE * 3 * INPUT_H * INPUT_W];  // ZED
    static float prob[BATCH_SIZE * OUTPUT_SIZE];
    IRuntime* runtime = createInferRuntime(gLogger);
    assert(runtime != nullptr);
    ICudaEngine* engine = runtime->deserializeCudaEngine(trtModelStream, size);
    assert(engine != nullptr);
    IExecutionContext* context = engine->createExecutionContext();
    assert(context != nullptr);
    delete[] trtModelStream;
    assert(engine->getNbBindings() == 2);
    void* buffers[2];
    // In order to bind the buffers, we need to know the names of the input and output tensors.
    // Note that indices are guaranteed to be less than IEngine::getNbBindings()
    const int inputIndex = engine->getBindingIndex(INPUT_BLOB_NAME);
    const int outputIndex = engine->getBindingIndex(OUTPUT_BLOB_NAME);
    assert(inputIndex == 0);
    assert(outputIndex == 1);
    // Create GPU buffers on device
    CUDA_CHECK(cudaMalloc((void**)&buffers[inputIndex], BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CUDA_CHECK(cudaMalloc((void**)&buffers[outputIndex], BATCH_SIZE * OUTPUT_SIZE * sizeof(float)));

    // Create stream
    cudaStream_t stream;
    CUDA_CHECK(cudaStreamCreate(&stream));

    // === ZED ===
    assert(BATCH_SIZE == 1); // This sample only support batch 1 for now

    sl::Mat left_sl, point_cloud;
    cv::Mat left_cv_rgb;
    sl::ObjectDetectionRuntimeParameters objectTracker_parameters_rt;
    sl::Objects objects;
    sl::Pose cam_w_pose;
    cam_w_pose.pose_data.setIdentity();

    // while (viewer.isAvailable()) {
    std::cout << "Starting data stream on " + std::string(ip) + ":" + std::to_string(port) << std::endl;
    while(true){
    // for(int k=0; k<10; ++k){
        if (zed.grab() == sl::ERROR_CODE::SUCCESS) {

            zed.retrieveImage(left_sl, sl::VIEW::LEFT);

            // Preparing inference
            cv::Mat left_cv_rgba = slMat2cvMat(left_sl);
            cv::cvtColor(left_cv_rgba, left_cv_rgb, cv::COLOR_BGRA2BGR);
            if (left_cv_rgb.empty()) continue;
            cv::Mat pr_img = preprocess_img(left_cv_rgb, INPUT_W, INPUT_H); // letterbox BGR to RGB
            int i = 0;
            int batch = 0;
            for (int row = 0; row < INPUT_H; ++row) {
                uchar* uc_pixel = pr_img.data + row * pr_img.step;
                for (int col = 0; col < INPUT_W; ++col) {
                    data[batch * 3 * INPUT_H * INPUT_W + i] = (float) uc_pixel[2] / 255.0;
                    data[batch * 3 * INPUT_H * INPUT_W + i + INPUT_H * INPUT_W] = (float) uc_pixel[1] / 255.0;
                    data[batch * 3 * INPUT_H * INPUT_W + i + 2 * INPUT_H * INPUT_W] = (float) uc_pixel[0] / 255.0;
                    uc_pixel += 3;
                    ++i;
                }
            }

            // Running inference
            doInference(*context, stream, buffers, data, prob, BATCH_SIZE);
            std::vector<std::vector < Yolo::Detection >> batch_res(BATCH_SIZE);
            auto& res = batch_res[batch];
            nms(res, &prob[batch * OUTPUT_SIZE], CONF_THRESH, NMS_THRESH);

            // Preparing for ZED SDK ingesting
            std::vector<sl::CustomBoxObjectData> objects_in;
            std::vector<int> labels;
            for (auto &it : res) {
                sl::CustomBoxObjectData tmp;
                cv::Rect r = get_rect(left_cv_rgb, it.bbox);
                // Fill the detections into the correct format
                tmp.unique_object_id = sl::generate_unique_id();
                tmp.probability = it.conf;
                tmp.label = (int) it.class_id;
                tmp.bounding_box_2d = cvt(r);
                tmp.is_grounded = false; //((int) it.class_id == 0); // Only the first class (person) is grounded, that is moving on the floor plane
                // others are tracked in full 3D space                
                objects_in.push_back(tmp);
                labels.push_back((int) it.class_id);
            }
            // Send the custom detected boxes to the ZED
            zed.ingestCustomBoxObjects(objects_in);


            // Displaying 'raw' objects
            // for (size_t j = 0; j < res.size(); j++) {
            //     cv::Rect r = get_rect(left_cv_rgb, res[j].bbox);
            //     cv::rectangle(left_cv_rgb, r, cv::Scalar(0x27, 0xC1, 0x36), 2);
            //     cv::putText(left_cv_rgb, std::to_string((int) res[j].class_id), cv::Point(r.x, r.y - 1), cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(0xFF, 0xFF, 0xFF), 2);
            // }
            // cv::imshow("Objects", left_cv_rgb);
            // cv::waitKey(10);

            // Retrieve the tracked objects, with 2D and 3D attributes
            zed.retrieveObjects(objects, objectTracker_parameters_rt);
            if(!objects.object_list.empty() && !labels.empty())
                tcpDispatch(objects.object_list, labels);
        }
    }
    // === ZED ===

    // Release stream and buffers
    cudaStreamDestroy(stream);
    CUDA_CHECK(cudaFree(buffers[inputIndex]));
    CUDA_CHECK(cudaFree(buffers[outputIndex]));
    // Destroy the engine
    context->destroy();
    engine->destroy();
    runtime->destroy();
    close(sock);

    return 0;
}

#include "common.h"

// common
QString savePath_{};
QString saveFormat_ = "yolo";
QString labelMode_{};
QMap<QString, bool> is_images_processed;
bool is_labeling_ = false;
std::map<int, std::string> clsList{{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "outpost"}, {7, "guard"}, {8, "base"}, {9, "negative"}};
std::vector<detectionLabel> detectionLabels_;
int saveId_ = 0;
bool autoMode_ = false;
bool labelSelected = false;
// cls
bool is_autoCut_ = false;
bool is_warp_ = false;
bool is_binary_ = false;
// autoMode
double confidence_threshold_ = 0.5;
double nms_threshold_ = 0.5;
std::vector<finalArmor> finalArmors_;
std::string modelSelection_{};
const std::string v8_model_path_ = getModelPath("v8.onnx");
const std::string v12_model_path_ = getModelPath("v12.onnx");
const std::string cls_model_path_ = getModelPath("number_classifier.onnx");
// detection
bool colorSave_ = false;
bool labelSave_ = false;
bool is_poseMode_ = false;

std::string getModelPath(const std::string modelPath)
{
    const char* appdir = std::getenv("APPDIR");
    std::string appdirStr = appdir ? appdir : "";
    return appdirStr.empty() ? "../model/" + modelPath : appdirStr + "/usr/share/DatasetMaker/model/" + modelPath;
}

cv::Point2f getCenterFromPose(std::vector<cv::Point2f> armor_points)
{
    double line1_k = (armor_points[2].y - armor_points[0].y) / (armor_points[2].x - armor_points[0].x + 1e-5);
    double line2_k = (armor_points[1].y - armor_points[3].y) / (armor_points[1].x - armor_points[3].x + 1e-5);
    double line1_b = armor_points[2].y - line1_k * armor_points[2].x;
    double line2_b = armor_points[1].y - line2_k * armor_points[1].x;

    cv::Point2f center;
    double center_x = (line1_b - line2_b) / (line2_k - line1_k + 1e-5);
    double center_y = line1_k * center_x + line1_b;
    center.x = static_cast<float>(center_x);
    center.y = static_cast<float>(center_y);

    return center;
}


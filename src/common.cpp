#include "common.h"

QString savePath_{};
QString saveFormat_ = "yolo";
QString labelMode_{};
bool is_labeling_ = false;
double confidence_threshold_ = 0.5;
double nms_threshold_ = 0.5;
std::vector<detectionLabel> detectionLabels_;
std::vector<finalArmor> finalArmors_;
bool is_autoCut_ = false;
int saveId_ = 0;
bool autoMode_ = false;
std::string modelSelection_{};
bool is_warp_ = false;
bool is_binary_ = false;
const std::string v8_model_path_ = getModelPath("v8.onnx");
const std::string v12_model_path_ = getModelPath("v12.onnx");
const std::string cls_model_path_ = getModelPath("number_classifier.onnx");
bool colorSave_ = false;

std::string getModelPath(const std::string modelPath)
{
    const char* appdir = std::getenv("APPDIR");
    std::string appdirStr = appdir ? appdir : "";
    return appdirStr.empty() ? "../model/" + modelPath : appdirStr + "/usr/share/DatasetMaker/model/" + modelPath;
}
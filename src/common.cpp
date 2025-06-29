#include "common.h"

QString savePath_{};
QString saveFormat_ = "yolo";
QString labelMode_{};
bool is_labeling_ = false;
double confidence_{};
std::vector<detectionLabel> detectionLabels_;
bool is_autoCut_{};

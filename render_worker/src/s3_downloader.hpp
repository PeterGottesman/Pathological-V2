#pragma once
#include <string>

// Downloads a file from an S3 bucket and saves it locally
bool DownloadSceneFromS3(const std::string& bucketName, 
                         const std::string& objectKey, 
                         const std::string& localFilePath);
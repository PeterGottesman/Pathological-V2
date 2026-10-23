#include "s3_downloader.hpp"
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <iostream>

bool DownloadSceneFromS3(const std::string& bucketName, 
                         const std::string& objectKey, 
                         const std::string& localFilePath) {
    
    Aws::Client::ClientConfiguration clientConfig;
    Aws::S3::S3Client s3Client(clientConfig);

    Aws::S3::Model::GetObjectRequest objectRequest;
    objectRequest.SetBucket(bucketName);
    objectRequest.SetKey(objectKey);

    auto getObjectOutcome = s3Client.GetObject(objectRequest);

    if (getObjectOutcome.IsSuccess()) {
        std::ofstream localFile(localFilePath, std::ios::binary);
        auto& s3Stream = getObjectOutcome.GetResultWithOwnership().GetBody();
        localFile << s3Stream.rdbuf();
        return true;
    } 
    
    auto error = getObjectOutcome.GetError();
    std::cerr << "S3 Download Failed: " << error.GetMessage() << std::endl;
    return false;
}
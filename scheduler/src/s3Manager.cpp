#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "s3Manager.hpp"

S3Manager::S3Manager(const S3Config &config) : config(config) {
  Aws::InitAPI(options);

  Aws::Client::ClientConfiguration clientConfig;

  clientConfig.region = config.reigon;

  if (!config.acceessKeyId.empty() && !config.secretKey.empty()) {
    Aws::Auth::AWSCredentials credentials(config.acceessKeyId,
                                          config.secretKey);

    client = std::make_unique<Aws::S3::S3Client>(credentials, clientConfig);
  } else {
    client = std::make_unique<Aws::S3::S3Client>(clientConfig);
  }
}

S3Manager::~S3Manager() {
  client.reset();
  Aws::ShutdownAPI(options);
}

bool S3Manager::validateFile(const std::string &s3Path) {
  Aws::S3::Model::HeadObjectRequest req;
  req.SetBucket(config.bucketName);
  req.SetKey(s3Path);

  auto resp = client->HeadObject(req);
  if (resp.IsSuccess()) {
    return true;
  }

  std::cerr << "Validation request failed for '" << s3Path
            << "': " << resp.GetError().GetMessage() << std::endl;

  return false;
}

std::optional<std::string>
S3Manager::requestSceneFile(const std::string &s3Path) {}

bool S3Manager::searchFiles(const std::string &s3Path) {}

std::optional<std::string>
S3Manager::requestOutputFile(const std::string &s3Path) {}

std::string S3Manager::extractFilename(const std::string &path) {}
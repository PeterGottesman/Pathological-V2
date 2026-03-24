#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <memory>
#include <optional>
#include <string>

struct S3Config {
  std::string bucketName;
  std::string reigon;
  std::string acceessKeyId;
  std::string secretKey;
};

class S3Manager {
public:
  explicit S3Manager(const S3Config &config);
  ~S3Manager();
  bool validateFile(const std::string &s3Path);
  std::optional<std::string> requestSceneFile(const std::string &s3Path);
  std::optional<std::string> requestOutputFile(const std::string &s3Path);

private:
  S3Config config;
  Aws::SDKOptions options;
  std::unique_ptr<Aws::S3::S3Client> client;

  std::string extractFilename(const std::string &path);
};
#pragma once

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSAllocator.h>
#include <aws/s3/S3Client.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct S3Config {
  std::string bucketName;
  std::string region;
  std::string profileName;
};

class S3Manager {
public:
  explicit S3Manager(const S3Config &config);
  ~S3Manager();

  bool keyExists(const std::string &s3Key);
  bool getObject(const std::string &s3Key, const std::string &localPath);
  bool putObject(const std::string &localPath, const std::string &s3Key,
                 bool overwriteExisting);
  std::string createLink(const std::string &s3Key);

  // Controls link expiration timer in seconds.
  static constexpr uint64_t kPresignedUrlTimeout = 1800;

private:
  S3Config config;
  Aws::SDKOptions options;
  std::unique_ptr<Aws::S3::S3Client> client;

  bool writeFileToPath(const std::string &path, std::streambuf *data,
                       long long expectedLen);
  std::shared_ptr<Aws::IOStream>
  readFileToAWSStream(const std::string &localPath);
};
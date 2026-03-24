#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct S3Config {
  std::string bucketName;
  std::string region;
};

class S3Manager {
public:
  explicit S3Manager(const S3Config &config);
  ~S3Manager();

  bool keyExists(const std::string &s3Key);
  bool getObject(const std::string &s3Key, const std::string &localPath);
  bool putObject(const std::string &localPath, const std::string &s3Key);
  std::string createLink(const std::string &outputKey);

  // Controls link expiration timer in seconds.
  static constexpr uint64_t kPresignedUrlTimeout = 1800;

private:
  S3Config config;
  Aws::SDKOptions options;
  std::unique_ptr<Aws::S3::S3Client> client;

  bool writeFileToPath(const std::string &path, std::streambuf *data,
                       long long expectedLen);

  void appendFileExtentsion(std::string &path);
};
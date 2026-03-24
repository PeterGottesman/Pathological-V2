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
  std::optional<std::string> requestFileUrl(const std::string &s3Key);

  static constexpr uint64_t kPresignedUrlTimeout = 1200;

private:
  S3Config config;
  Aws::SDKOptions options;
  std::unique_ptr<Aws::S3::S3Client> client;
};
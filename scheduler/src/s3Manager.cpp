#include <aws/s3/model/HeadObjectRequest.h>
#include <iostream>

#include "s3Manager.hpp"

S3Manager::S3Manager(const S3Config &config) : config_(config) {
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = config.region;
  client_ = std::make_unique<Aws::S3::S3Client>(clientConfig);
}

S3Manager::~S3Manager() {
  client_.reset();
  Aws::ShutdownAPI(options);
}

bool S3Manager::keyExists(const std::string &s3Key) {
  std::cout << "Fetching key: " << s3Key << std::endl;
  Aws::S3::Model::HeadObjectRequest req;
  req.SetBucket(config_.bucketName);
  req.SetKey(s3Key);

  auto resp = client_->HeadObject(req);
  if (resp.IsSuccess()) {
    return true;
  }

  std::cerr << "Validation request failed for '" << s3Key
            << "': " << resp.GetError().GetMessage() << std::endl;

  return false;
}

std::optional<std::string> S3Manager::requestFileUrl(const std::string &s3Key) {
  auto url = client_->GeneratePresignedUrl(config_.bucketName, s3Key,
                                           Aws::Http::HttpMethod::HTTP_GET,
                                           kPresignedUrlTimeout);
  return url.empty() ? std::nullopt : std::optional<std::string>(url);
}
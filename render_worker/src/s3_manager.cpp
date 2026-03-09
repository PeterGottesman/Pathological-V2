#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "s3_manager.hpp"

S3Manager::S3Manager(const S3Config &config) : config(config) {
  Aws::InitAPI(options);
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = config.region;
  client = std::make_unique<Aws::S3::S3Client>(clientConfig);
}

S3Manager::~S3Manager() {
  client.reset();
  Aws::ShutdownAPI(options);
}

bool S3Manager::keyExists(const std::string &s3Key) {
  Aws::S3::Model::HeadObjectRequest req;
  req.SetBucket(config.bucketName);
  req.SetKey(s3Key);

  auto resp = client->HeadObject(req);
  if (resp.IsSuccess()) {
    return true;
  }

  std::cerr << "Validation request failed for '" << s3Key
            << "': " << resp.GetError().GetMessage() << std::endl;

  return false;
}

bool S3Manager::getObject(const std::string &s3Key,
                          const std::string &localPath) {
  Aws::S3::Model::GetObjectRequest req;
  req.SetBucket(config.bucketName);
  req.SetKey(s3Key);

  auto outcome = client->GetObject(req);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = outcome.GetError();
    std::cerr << "Error during GET object request for key: '" << s3Key << "'. "
              << err.GetExceptionName() << ": " << err.GetMessage()
              << std::endl;
    return false;
  } else {
    std::cout << "Successfully retrieved: '" << s3Key << "' from "
              << config.bucketName << "." << std::endl;
  }

  auto &result = outcome.GetResult();
  auto objectLen = result.GetContentLength();

  return writeFileToPath(localPath, result.GetBody().rdbuf(), objectLen);
}

bool S3Manager::putObject(const std::string &localPath,
                          const std::string &s3Key) {}

std::string S3Manager::createLink(const std::string &outputKey) {}

// Function that writes stream of data in binary mode. Designed to use for gltf
// files and png files.
bool S3Manager::writeFileToPath(const std::string &path, std::streambuf *data,
                                long long expectedLen = 1) {
  // Open file in binary mode
  std::ofstream out(path, std::ios::out | std::ios::binary);

  if (!out.is_open()) {
    std::cerr << "Error: Could not open file " << path << " for writing."
              << std::endl;
    return false;
  }
  std::copy(std::istreambuf_iterator<char>(data), // Read data stream
            std::istreambuf_iterator<char>(),     // End of stream
            std::ostreambuf_iterator<char>(out)); // Writing stream to file

  if (!out.good()) {
    std::cerr << "Failed to write file to local path: " << path << std::endl;
    return false;
  }

  // Check content length matches S3 length
  if (expectedLen >= 0) {
    auto actualLen = static_cast<long long>(std::filesystem::file_size(path));
    if (actualLen != expectedLen) {
      std::cerr << "File size mismatch: Expected " << expectedLen
                << " bytes. Got " << actualLen << " bytes." << std::endl;
      return false;
    }
  }

  return true;
}

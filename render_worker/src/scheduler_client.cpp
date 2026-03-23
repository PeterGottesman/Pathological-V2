#include "scheduler_client.hpp"

int SchedulerClient::EstablishConnection() {
  // This populates a request from the server with the worker's info.

  WorkerInfo request;
  request.set_worker_ip(worker_ip_);
  request.set_worker_id(worker_id_);
  request.set_port(port_);

  ServerResponse response;
  ClientContext context;

  // The actual RPC.

  Status status = stub_->EstablishConnection(&context, request, &response);

  // If the RPC call succeeds, an output message is displayed detailing the worker's current status with the scheduler.

  if (status.ok()) {
    if (response.status() == RegistrationStatus::OK) {
      if (response.has_assigned_id()) {
        worker_id_ = response.assigned_id();
        std::cout << "Registered with scheduler, assigned ID: " << worker_id_ << std::endl;
      }
    } else if (response.status() == RegistrationStatus::RECONNECTED) {
        if (response.has_assigned_id()) {
          std::cout << "Reconnected to scheduler with ID: " << worker_id_ << std::endl;
        }
    } else if (response.status() == RegistrationStatus::REJECTED) {
        std::cout << "Registration rejected by scheduler." << std::endl;
    }
    return 0;
  } else {
    std::cerr << "EstablishConnection failed: " << status.error_message() << std::endl;
    return 1;
  }
}

void SchedulerClient::Heartbeat() {
  WorkerID request;
  request.set_worker_id(worker_id_);

  ServerResponse response;
  ClientContext context;

  Status status = stub_->Heartbeat(&context, request, &response);
  if (status.ok()) {
    if (response.status() == RegistrationStatus::UNKNOWN) {
      std::cout << "Scheduler does not recognise this worker, " << "re-registration required." << std::endl;
    }
  } else {
    std::cerr << "Heartbeat failed: " << status.error_message() << std::endl;
  }
}

void SchedulerClient::JobCompleted(int32_t job_id) {
  JobCompletedRequest request;
  request.set_worker_id(worker_id_);
  request.set_job_id(job_id);

  ServerResponse response;
  ClientContext context;

  Status status = stub_->JobCompleted(&context, request, &response);
  if (status.ok()) {
    std::cout << "Job " << job_id << " completion acknowledged." << std::endl;
  } else {
    std::cerr << "JobCompleted failed: " << status.error_message() << std::endl;
    }
}

void SchedulerClient::Disconnect() {
  WorkerID request;
  request.set_worker_id(worker_id_);

  ServerResponse response;
  ClientContext context;

  Status status = stub_->Disconnect(&context, request, &response);
  if (status.ok()) {
    std::cout << "Disconnected from scheduler." << std::endl;
  } else {
    std::cerr << "Disconnect failed: " << status.error_message() << std::endl;
  }
}

//snippet-sourcedescription:[put_object.cpp demonstrates how to asynchronously put a file into an Amazon S3 bucket.]
//snippet-service:[s3]
//snippet-keyword:[Amazon S3]
//snippet-keyword:[C++]
//snippet-keyword:[Code Sample]
//snippet-sourcetype:[snippet]
//snippet-sourceauthor:[AWS]

/*
   Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.

   This file is licensed under the Apache License, Version 2.0 (the "License").
   You may not use this file except in compliance with the License. A copy of
   the License is located at

    http://aws.amazon.com/apache2.0/

   This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied. See the License for the
   specific language governing permissions and limitations under the License.
*/

//snippet-start:[s3.cpp.put_object_async.inc]
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sys/stat.h>
#include <thread>
//snippet-end:[s3.cpp.put_object_async.inc]

/**
 * Check if file exists
 *
 * Note: If using C++17, can use std::filesystem::exists()
 */
inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

/**
 * Function called when PutObjectAsync() finishes
 *
 * The thread that started the async operation is waiting for the operation
 * to finish. A std::condition_variable is used to communicate between the
 * two threads.
*/
// snippet-start:[s3.cpp.put_object_async_finished.code]
std::mutex upload_mutex;
std::condition_variable upload_variable;
bool upload_finished = false;

void put_object_async_finished(const Aws::S3::S3Client* client, 
    const Aws::S3::Model::PutObjectRequest& request, 
    const Aws::S3::Model::PutObjectOutcome& outcome,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    // Output operation status
    if (outcome.IsSuccess()) {
        std::cout << "Finished uploading " << context->GetUUID() << std::endl;
    }
    else {
        auto error = outcome.GetError();
        std::cout << "ERROR: " << error.GetExceptionName() << ": "
            << error.GetMessage() << std::endl;
    }

    // Update global flag and notify waiting function
    std::unique_lock<std::mutex> lock(upload_mutex);
    upload_finished = true;
    lock.unlock();
    upload_variable.notify_one();
}
// snippet-end:[s3.cpp.put_object_async_finished.code]

/**
 * Asynchronously put an object into an Amazon S3 bucket
 */
// snippet-start:[s3.cpp.put_object_async.code]
bool put_s3_object_async(const Aws::String& s3_bucket_name,
    const Aws::String& s3_object_name,
    const std::string& file_name,
    const Aws::String& region = "")
{
    // Verify file_name exists
    if (!file_exists(file_name)) {
        std::cout << "ERROR: NoSuchFile: The specified file does not exist"
            << std::endl;
        return false;
    }

    // If region is specified, use it
    Aws::Client::ClientConfiguration clientConfig;
    if (!region.empty())
        clientConfig.region = region;

    // Set up request
    Aws::S3::S3Client s3_client(clientConfig);
    Aws::S3::Model::PutObjectRequest object_request;

    object_request.SetBucket(s3_bucket_name);
    object_request.SetKey(s3_object_name);
    const std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
            file_name.c_str(),
            std::ios_base::in | std::ios_base::binary);
    object_request.SetBody(input_data);
    auto context =
        Aws::MakeShared<Aws::Client::AsyncCallerContext>("PutObjectAllocationTag");
    context->SetUUID(s3_object_name);

    // Put the object asynchronously
    s3_client.PutObjectAsync(object_request, 
                             put_object_async_finished,
                             context);
    return true;
    // snippet-end:[s3.cpp.put_object_async.code]
}

/**
 * Exercise put_s3_object_async()
 */
int main(int argc, char** argv)
{

    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        // Assign these values before running the program
        const Aws::String bucket_name = "bucket-name-scalwas";
        const Aws::String object_name = "xyplorer_full.zip";
        const std::string file_name = "\\EraseMe\\xyplorer_full.zip";
        const Aws::String region = "";      // Optional

        // Put the file into the S3 bucket
        std::unique_lock<std::mutex> lock(upload_mutex);
        upload_finished = false;
        if (put_s3_object_async(bucket_name, object_name, file_name, region)) {
            // Wait for upload to finish
            std::cout << "Waiting for file upload to complete..." << std::endl;
            upload_variable.wait(lock, []{return upload_finished;});
#if 0
            // Temporarily remove mutex/condition_variable; sleep instead
            upload_finished = true;
            while (!upload_finished) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
#endif
            std::cout << "File upload completed" << std::endl;
            // We can terminate the program now
        }
    }
    Aws::ShutdownAPI(options);
}

 
//snippet-sourcedescription:[set_acl.cpp demonstrates how to retrieve and modify the access control list of an Amazon S3 bucket or object.]
//snippet-service:[s3]
//snippet-keyword:[Amazon S3]
//snippet-keyword:[C++]
//snippet-keyword:[Code Sample]
//snippet-sourcetype:[full-example]
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

//snippet-start:[s3.cpp.set_acl.inc]
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/AccessControlPolicy.h>
#include <aws/s3/model/GetBucketAclRequest.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <aws/s3/model/GetObjectAclRequest.h>
#include <aws/s3/model/PutObjectAclRequest.h>
#include <aws/s3/model/Grant.h>
#include <aws/s3/model/Grantee.h>
#include <aws/s3/model/Permission.h>
//snippet-end:[s3.cpp.set_acl.inc]

Aws::S3::Model::Permission GetPermission(Aws::String access)
{
    if (access == "FULL_CONTROL")
        return Aws::S3::Model::Permission::FULL_CONTROL;
    if (access == "WRITE")
        return Aws::S3::Model::Permission::WRITE;
    if (access == "READ")
        return Aws::S3::Model::Permission::READ;
    if (access == "WRITE_ACP")
        return Aws::S3::Model::Permission::WRITE_ACP;
    if (access == "READ_ACP")
        return Aws::S3::Model::Permission::READ_ACP;
    return Aws::S3::Model::Permission::NOT_SET;
}

void SetAclForBucket(Aws::String bucket_name,
    Aws::String grantee_id,
    Aws::String permission)
{
    // snippet-start:[s3.cpp.set_acl_bucket.code]
    // Set up the get request
    Aws::S3::S3Client s3_client;
    Aws::S3::Model::GetBucketAclRequest get_request;
    get_request.SetBucket(bucket_name);

    // Get the current access control policy
    auto get_outcome = s3_client.GetBucketAcl(get_request);
    if (!get_outcome.IsSuccess())
    {
        auto error = get_outcome.GetError();
        std::cout << "Original GetBucketAcl error: " << error.GetExceptionName()
            << " - " << error.GetMessage() << std::endl;
        return;
    }

    // Reference the retrieved access control policy
    auto result = get_outcome.GetResult();

    // Copy the result to an access control policy object (cannot type cast)
    Aws::S3::Model::AccessControlPolicy acp;
    acp.SetOwner(result.GetOwner());

    acp.SetGrants(result.GetGrants());        // creates const Vector<Grants>
    // Make non-const copy of Vector<Grants> with hard-set grantee type
    auto &acp_grants = result.GetGrants();
    Aws::Vector<Aws::S3::Model::Grant> updated_grants;
    for (auto acp_grant : result.GetGrants())
    {
        std::shared_ptr<Aws::S3::Model::Grant> updated_grant = std::make_shared<Aws::S3::Model::Grant>();
        std::shared_ptr<Aws::S3::Model::Grantee> updated_grantee = std::make_shared<Aws::S3::Model::Grantee>();

        // Copy current grant permission
        updated_grant->SetPermission(acp_grant.GetPermission());

        // Copy grantee fields
        //auto original_grantee = acp_grant.GetGrantee();
        *updated_grantee = acp_grant.GetGrantee();

        // Grantee Type is required
        updated_grantee->SetType(Aws::S3::Model::Type::CanonicalUser);

        // Save updated grantee to updated grant
        updated_grant->SetGrantee(*updated_grantee);

        // Add updated_grant to vector
        updated_grants.push_back(*updated_grant);
    }

    // Add new grant
    Aws::S3::Model::Grant new_grant;
    Aws::S3::Model::Grantee new_grantee;
    new_grantee.SetID(grantee_id);
    new_grantee.SetType(Aws::S3::Model::Type::CanonicalUser);
    new_grant.SetGrantee(new_grantee);
    new_grant.SetPermission(GetPermission(permission));
    updated_grants.push_back(new_grant);
    // If we had not needed to recreate the Vector<Grants>, we could
    // have just added new_grant to the vector
    // acp.AddGrants(new_grant);

    // Set the updated grants to the ACP
    acp.SetGrants(updated_grants);

    // Set up the put request
    Aws::S3::Model::PutBucketAclRequest put_request;
    put_request.SetAccessControlPolicy(acp);
    put_request.SetBucket(bucket_name);

    // Set the new access control policy
    auto set_outcome = s3_client.PutBucketAcl(put_request);
    // snippet-end:[s3.cpp.set_acl_bucket.code]
    if (!set_outcome.IsSuccess())
    {
        auto error = set_outcome.GetError();
        std::cout << "PutBucketAcl error: " << error.GetExceptionName() 
            << " - " << error.GetMessage() << std::endl;
        return;
    }

    // Verify the operation by retrieving the updated ACP
    get_outcome = s3_client.GetBucketAcl(get_request);
    if (!get_outcome.IsSuccess())
    {
        auto error = get_outcome.GetError();
        std::cout << "Updated GetBucketAcl error: " << error.GetExceptionName()
            << " - " << error.GetMessage() << std::endl;
        return;
    }
    result = get_outcome.GetResult();

    // Output some settings of the updated ACP
    std::cout << "Updated Bucket ACL:\n";
    auto grants = result.GetGrants();
    for (auto & grant : grants) {
        auto grantee = grant.GetGrantee();
        std::cout << "  Grantee Display Name: " 
            << grantee.GetDisplayName() << std::endl;

        std::cout << "  Permission: ";
        auto perm = grant.GetPermission();
        switch (perm)
        {
        case Aws::S3::Model::Permission::NOT_SET:
            std::cout << "NOT_SET\n";
            break;
        case Aws::S3::Model::Permission::FULL_CONTROL:
            std::cout << "FULL_CONTROL\n";
            break;
        case Aws::S3::Model::Permission::WRITE:
            std::cout << "WRITE\n";
            break;
        case Aws::S3::Model::Permission::WRITE_ACP:
            std::cout << "WRITE_ACP\n";
            break;
        case Aws::S3::Model::Permission::READ:
            std::cout << "READ\n";
            break;
        case Aws::S3::Model::Permission::READ_ACP:
            std::cout << "READ_ACP\n";
            break;
        default:
            std::cout << "UNKNOWN VALUE\n";
            break;
        }
    }
}

void SetAclForObject(Aws::String bucket_name, 
    Aws::String object_name,
    Aws::String grantee_id, 
    Aws::String permission)
{
    // snippet-start:[s3.cpp.set_acl_object.code]
    // Set up the get request
    Aws::S3::S3Client s3_client;
    Aws::S3::Model::GetObjectAclRequest get_request;
    get_request.SetBucket(bucket_name);
    get_request.SetKey(object_name);

    // Get the current access control policy
    auto get_outcome = s3_client.GetObjectAcl(get_request);
    if (!get_outcome.IsSuccess())
    {
        auto error = get_outcome.GetError();
        std::cout << "Original GetObjectAcl error: " << error.GetExceptionName()
            << " - " << error.GetMessage() << std::endl;
        return;
    }

    // Reference the retrieved access control policy
    auto result = get_outcome.GetResult();

    // Copy the result to an access control policy object (cannot type cast)
    Aws::S3::Model::AccessControlPolicy acp;
    acp.SetOwner(result.GetOwner());

    //acp.SetGrants(result.GetGrants());        // creates const Vector<Grants>
     // Make non-const copy of Vector<Grants> with hard-set grantee type
    auto &acp_grants = result.GetGrants();
    Aws::Vector<Aws::S3::Model::Grant> updated_grants;
    for (auto acp_grant : result.GetGrants())
    {
        std::shared_ptr<Aws::S3::Model::Grant> updated_grant = std::make_shared<Aws::S3::Model::Grant>();
        std::shared_ptr<Aws::S3::Model::Grantee> updated_grantee = std::make_shared<Aws::S3::Model::Grantee>();

        // Copy current grant permission
        updated_grant->SetPermission(acp_grant.GetPermission());

        // Copy grantee fields
        //auto original_grantee = acp_grant.GetGrantee();
        *updated_grantee = acp_grant.GetGrantee();

        // Grantee Type is required
        updated_grantee->SetType(Aws::S3::Model::Type::CanonicalUser);

        // Save updated grantee to updated grant
        updated_grant->SetGrantee(*updated_grantee);

        // Add updated_grant to vector
        updated_grants.push_back(*updated_grant);
}

    // Add new grant
    Aws::S3::Model::Grant new_grant;
    Aws::S3::Model::Grantee new_grantee;
    new_grantee.SetID(grantee_id);
    new_grantee.SetType(Aws::S3::Model::Type::CanonicalUser);
    new_grant.SetGrantee(new_grantee);
    new_grant.SetPermission(GetPermission(permission));
    updated_grants.push_back(new_grant);
    // If we had not needed to recreate the Vector<Grants>, we could
    // have just added new_grant to the vector
    // acp.AddGrants(new_grant);

    // Set the updated grants to the ACP
    acp.SetGrants(updated_grants);

    // Set up the put request
    Aws::S3::Model::PutObjectAclRequest put_request;
    put_request.SetAccessControlPolicy(acp);
    put_request.SetBucket(bucket_name);
    put_request.SetKey(object_name);

    // Set the new access control policy
    auto set_outcome = s3_client.PutObjectAcl(put_request);
    // snippet-end:[s3.cpp.set_acl_object.code]
    if (!set_outcome.IsSuccess())
    {
        auto error = set_outcome.GetError();
        std::cout << "PutObjectAcl error: " << error.GetExceptionName()
            << " - " << error.GetMessage() << std::endl;
        return;
    }
}

/**
 * Exercise SetAclForBucket() and SetAclForObject()
 */
int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        // Assign these values before compiling the program
        const Aws::String bucket_name = "BUCKET_NAME";
        const Aws::String object_name = "OBJECT_NAME";
        const Aws::String grantee_id = "AWS_USER_ID";
        const Aws::String permission = "READ";

        // Set the access control lists for a bucket and an object
        //SetAclForBucket(bucket_name, grantee_id, permission);
        SetAclForObject(bucket_name, object_name, grantee_id, permission);
    }
    Aws::ShutdownAPI(options);
}


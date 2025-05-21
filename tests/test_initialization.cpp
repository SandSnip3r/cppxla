#include "gtest/gtest.h"
#include "pjrt/context.hpp"
#include "pjrt/client.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

namespace {

TEST(PjrtInitialization, ContextCreation) {
    SCOPED_TRACE("Testing pjrt::Context creation");
    ASSERT_NO_THROW({
        pjrt::Context context;
        // Assuming context.pjrtApi_ is accessible for this check, similar to your main.cpp
        ASSERT_NE(context.pjrtApi_, nullptr) << "pjrt::Context created, but context.pjrtApi_ is null.";
    }) << "pjrt::Context constructor threw an exception.";
    SUCCEED() << "pjrt::Context created successfully and pjrtApi_ is not null.";
}

TEST(PjrtInitialization, ClientCreation) {
    SCOPED_TRACE("Testing pjrt::Client creation");
    // Each TEST is independent, so create a context here.
    // If Context creation is expensive and used by many tests, a Test Fixture (TEST_F) would be better.
    pjrt::Context context; // Assuming this succeeds based on the previous test or it will throw.
                          // For strict independence, you might wrap this in ASSERT_NO_THROW as well.

    ASSERT_NO_THROW({
        pjrt::Client client(context);
        // You could add more specific assertions about the client if needed.
    }) << "pjrt::Client constructor threw an exception.";
    SUCCEED() << "pjrt::Client created successfully.";
}

TEST(PjrtInitialization, ApiVersionCheck) {
    SCOPED_TRACE("Testing PJRT API version access");
    pjrt::Context context;
    ASSERT_NE(context.pjrtApi_, nullptr) << "Context creation failed or pjrtApi_ is null, cannot check version.";

    // Log the version for information
    std::cout << "  [INFO] PJRT C API Version: "
              << context.apiMajorVersion() << "."
              << context.apiMinorVersion() << std::endl;

    // Example of an actual test:
    // EXPECT_GT(version->major_version, 0); // Or some other meaningful check
    // For this first test, just ensuring it doesn't crash and logging is a start.
    // If version is 0.0, it might be okay or an issue depending on plugin.
    if (context.apiMajorVersion() == 0 && context.apiMinorVersion() == 0) {
        ADD_FAILURE() << "PJRT API version is 0.0. This might indicate an uninitialized struct by the plugin.";
    } else {
        SUCCEED() << "PJRT API version accessed: " << context.apiMajorVersion() << "." << context.apiMinorVersion();
    }
}

TEST(PjrtInitialization, PlatformNameCheck) {
    SCOPED_TRACE("Testing client platform name retrieval");
    pjrt::Context context;
    pjrt::Client client(context);
    std::string platformName;

    ASSERT_NO_THROW({
        platformName = client.platformName();
    }) << "client.platformName() threw an exception.";

    std::cout << "  [INFO] Client Platform Name: \"" << platformName << "\"" << std::endl;

    // Example of a more useful check:
    // EXPECT_FALSE(platformName.empty()) << "Client Platform Name should not be empty for a functional plugin.";
    // For now, just checking it can be called.
    if (platformName.empty()) {
        // This is not necessarily a failure for all plugins, but good to note.
        std::cout << "  [NOTE] Client Platform Name is empty." << std::endl;
    }
    SUCCEED() << "client.platformName() called successfully.";
}

} // anonymous namespace
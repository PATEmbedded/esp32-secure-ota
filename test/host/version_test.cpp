// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar

#include <gtest/gtest.h>

#include <cstddef>
#include <iterator>
#include <string>

#include "secure_ota_version.h"

namespace {

sota_version_t parse(const char* text) {
    sota_version_t version{};
    EXPECT_TRUE(sota_version_parse(text, &version)) << text;
    return version;
}

int compare(const char* a, const char* b) {
    const sota_version_t version_a = parse(a);
    const sota_version_t version_b = parse(b);
    return sota_version_compare(&version_a, &version_b);
}

TEST(VersionParse, ReadsNumbersAndPrerelease) {
    const sota_version_t v = parse("v1.20.3-rc.1+build.7");
    EXPECT_EQ(v.major, 1u);
    EXPECT_EQ(v.minor, 20u);
    EXPECT_EQ(v.patch, 3u);
    EXPECT_STREQ(v.prerelease, "rc.1");
}

TEST(VersionParse, ReleaseHasEmptyPrerelease) {
    EXPECT_STREQ(parse("2.0.0").prerelease, "");
    EXPECT_STREQ(parse("2.0.0+20261016").prerelease, "");
}

TEST(VersionParse, AcceptsHyphensInPrerelease) {
    EXPECT_STREQ(parse("1.0.0-x-y-z.--").prerelease, "x-y-z.--");
}

TEST(VersionParse, RejectsMalformedVersions) {
    const char* const malformed[] = {
        "",       "1",           "1.2",         "1.2.3.4",  "01.2.3",         "1.02.3",       "1.2.03",
        "1.2.3-", "1.2.3-rc..1", "1.2.3-01",    "1.2.3+",   "1.2.3+build..1", "1.2.3 ",       " 1.2.3",
        "a.b.c",  "1.2.3-rc_1",  "1.2.3-rc+a!", "-1.2.3",   "4294967296.0.0", "1.2.3-rc.1+b",
    };
    for (const char* text : malformed) {
        if (std::string(text) == "1.2.3-rc.1+b") {
            continue;  // valid, listed to show that build metadata may follow a pre-release
        }
        sota_version_t v{};
        EXPECT_FALSE(sota_version_parse(text, &v)) << '"' << text << '"';
    }
}

TEST(VersionParse, RejectsNullArguments) {
    sota_version_t v{};
    EXPECT_FALSE(sota_version_parse(nullptr, &v));
    EXPECT_FALSE(sota_version_parse("1.0.0", nullptr));
}

TEST(VersionParse, LimitsPrereleaseLength) {
    sota_version_t v{};
    const std::string longest = "1.0.0-" + std::string(SOTA_PRERELEASE_MAX, 'a');
    const std::string too_long = "1.0.0-" + std::string(SOTA_PRERELEASE_MAX + 1, 'a');
    EXPECT_TRUE(sota_version_parse(longest.c_str(), &v));
    EXPECT_FALSE(sota_version_parse(too_long.c_str(), &v));
}

TEST(VersionParse, AcceptsLargestNumber) {
    EXPECT_EQ(parse("4294967295.0.0").major, 4294967295u);
}

TEST(VersionCompare, OrdersNumbersNumerically) {
    EXPECT_LT(compare("1.9.0", "1.10.0"), 0);
    EXPECT_LT(compare("1.0.9", "1.0.10"), 0);
    EXPECT_GT(compare("2.0.0", "1.99.99"), 0);
    EXPECT_EQ(compare("1.2.3", "v1.2.3"), 0);
}

TEST(VersionCompare, FollowsSemverPrecedence) {
    // The example list from section 11 of the Semantic Versioning 2.0.0 specification.
    const char* const ordered[] = {
        "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta",
        "1.0.0-beta.2", "1.0.0-beta.11", "1.0.0-rc.1",       "1.0.0",
    };
    for (std::size_t i = 0; i + 1 < std::size(ordered); ++i) {
        EXPECT_LT(compare(ordered[i], ordered[i + 1]), 0) << ordered[i] << " < " << ordered[i + 1];
        EXPECT_GT(compare(ordered[i + 1], ordered[i]), 0) << ordered[i + 1] << " > " << ordered[i];
    }
}

TEST(VersionCompare, IgnoresBuildMetadata) {
    EXPECT_EQ(compare("1.0.0+build.1", "1.0.0+build.2"), 0);
    EXPECT_EQ(compare("1.0.0-rc.1+a", "1.0.0-rc.1"), 0);
}

TEST(Decide, InstallsNewerVersionOfTheSameProject) {
    EXPECT_EQ(sota_decide("demo", "1.0.0", "demo", "1.0.1"), SOTA_INSTALL);
    EXPECT_EQ(sota_decide("demo", "1.0.0-rc.2", "demo", "1.0.0"), SOTA_INSTALL);
}

TEST(Decide, SkipsTheSameAndOlderVersions) {
    EXPECT_EQ(sota_decide("demo", "1.2.0", "demo", "1.2.0"), SOTA_NOT_NEWER);
    EXPECT_EQ(sota_decide("demo", "1.2.0", "demo", "1.1.9"), SOTA_NOT_NEWER);
    EXPECT_EQ(sota_decide("demo", "1.2.0", "demo", "1.2.0-rc.1"), SOTA_NOT_NEWER);
}

TEST(Decide, RejectsImagesOfOtherProjects) {
    EXPECT_EQ(sota_decide("demo", "1.0.0", "other", "9.0.0"), SOTA_OTHER_PROJECT);
}

TEST(Decide, RejectsVersionsThatAreNotSemantic) {
    EXPECT_EQ(sota_decide("demo", "1.0.0", "demo", "latest"), SOTA_BAD_VERSION);
    EXPECT_EQ(sota_decide("demo", "v1.0-3-g1a2b3c4", "demo", "1.0.1"), SOTA_BAD_VERSION);
}

}  // namespace

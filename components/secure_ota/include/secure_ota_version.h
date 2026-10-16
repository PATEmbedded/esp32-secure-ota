// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar
//
// Semantic version parsing and the decision whether an update image may be
// installed. Plain C without ESP-IDF dependencies, so it is unit tested on a PC.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Longest pre-release tag that is kept, without the terminating zero. */
#define SOTA_PRERELEASE_MAX 31

/** Size of esp_app_desc_t::project_name, including the terminating zero. */
#define SOTA_PROJECT_NAME_SIZE 32

/** A semantic version (https://semver.org). Build metadata is dropped. */
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    char prerelease[SOTA_PRERELEASE_MAX + 1]; /**< Empty for a release. */
} sota_version_t;

/**
 * Parses "1.2.3", "v1.2.3", "1.2.3-rc.1" or "1.2.3+build.5".
 *
 * @return false for anything that is not a semantic version, or when the
 *         pre-release tag is longer than SOTA_PRERELEASE_MAX characters.
 */
bool sota_version_parse(const char *text, sota_version_t *version);

/**
 * Compares two versions by semver precedence.
 *
 * @return a negative number, zero or a positive number when `a` is older than,
 *         equal to or newer than `b`.
 */
int sota_version_compare(const sota_version_t *a, const sota_version_t *b);

/** Outcome of sota_decide(). */
typedef enum {
    SOTA_INSTALL,       /**< The candidate is newer than the running app. */
    SOTA_NOT_NEWER,     /**< The candidate is the same version or older. */
    SOTA_OTHER_PROJECT, /**< The candidate was built for another project. */
    SOTA_BAD_VERSION,   /**< One of the versions is not a semantic version. */
} sota_decision_t;

/** Decides whether a candidate image may replace the running app. */
sota_decision_t sota_decide(const char *running_project, const char *running_version,
                            const char *candidate_project, const char *candidate_version);

#ifdef __cplusplus
}
#endif

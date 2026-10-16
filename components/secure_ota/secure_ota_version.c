// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Pourya Afshintabar

#include "secure_ota_version.h"

#include <stddef.h>
#include <string.h>

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_identifier_char(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
}

static bool all_digits(const char *text, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (!is_digit(text[i])) {
            return false;
        }
    }
    return true;
}

static int compare_numbers(uint64_t a, uint64_t b) {
    return (a > b) - (a < b);
}

/* Reads a number without leading zeros that fits in 32 bits. */
static bool parse_number(const char **cursor, uint32_t *value) {
    const char *p = *cursor;
    if (!is_digit(p[0]) || (p[0] == '0' && is_digit(p[1]))) {
        return false;
    }
    uint64_t result = 0;
    while (is_digit(*p)) {
        result = result * 10u + (uint64_t)(*p - '0');
        if (result > UINT32_MAX) {
            return false;
        }
        ++p;
    }
    *value = (uint32_t)result;
    *cursor = p;
    return true;
}

static bool skip(const char **cursor, char expected) {
    if (**cursor != expected) {
        return false;
    }
    ++*cursor;
    return true;
}

/*
 * Checks a dot-separated list of identifiers made of [0-9A-Za-z-]. Numeric
 * identifiers of a pre-release tag must not have leading zeros; build
 * metadata may.
 */
static bool valid_identifiers(const char *text, size_t length, bool no_leading_zeros) {
    size_t start = 0;
    for (size_t i = 0; i <= length; ++i) {
        if (i == length || text[i] == '.') {
            const size_t identifier_length = i - start;
            if (identifier_length == 0) {
                return false;
            }
            if (no_leading_zeros && identifier_length > 1 && text[start] == '0' &&
                all_digits(text + start, identifier_length)) {
                return false;
            }
            start = i + 1;
        } else if (!is_identifier_char(text[i])) {
            return false;
        }
    }
    return true;
}

bool sota_version_parse(const char *text, sota_version_t *version) {
    if (text == NULL || version == NULL) {
        return false;
    }

    sota_version_t parsed = {0};
    const char *p = text;
    if (*p == 'v' || *p == 'V') {
        ++p;
    }
    if (!parse_number(&p, &parsed.major) || !skip(&p, '.') || !parse_number(&p, &parsed.minor) ||
        !skip(&p, '.') || !parse_number(&p, &parsed.patch)) {
        return false;
    }

    if (*p == '-') {
        ++p;
        const size_t length = strcspn(p, "+");
        if (length > SOTA_PRERELEASE_MAX || !valid_identifiers(p, length, true)) {
            return false;
        }
        memcpy(parsed.prerelease, p, length);
        parsed.prerelease[length] = '\0';
        p += length;
    }

    if (*p == '+') {
        ++p;
        const size_t length = strlen(p);
        if (!valid_identifiers(p, length, false)) {
            return false;
        }
        p += length;
    }

    if (*p != '\0') {
        return false;
    }
    *version = parsed;
    return true;
}

/* Compares pre-release tags by the precedence rules in section 11 of semver 2.0.0. */
static int compare_prerelease(const char *a, const char *b) {
    if (*a == '\0' || *b == '\0') {
        return (*a == '\0') - (*b == '\0'); /* a release ranks above any pre-release */
    }
    for (;;) {
        const size_t length_a = strcspn(a, ".");
        const size_t length_b = strcspn(b, ".");
        const bool numeric_a = all_digits(a, length_a);
        const bool numeric_b = all_digits(b, length_b);

        int order;
        if (numeric_a && numeric_b) {
            /* Without leading zeros, the longer number is the larger one. */
            order = length_a != length_b ? compare_numbers(length_a, length_b) : strncmp(a, b, length_a);
        } else if (numeric_a != numeric_b) {
            order = numeric_a ? -1 : 1; /* numeric identifiers rank below alphanumeric ones */
        } else {
            const size_t shorter = length_a < length_b ? length_a : length_b;
            order = strncmp(a, b, shorter);
            if (order == 0) {
                order = compare_numbers(length_a, length_b);
            }
        }
        if (order != 0) {
            return order < 0 ? -1 : 1;
        }

        a += length_a;
        b += length_b;
        if (*a == '\0' || *b == '\0') {
            return (*a != '\0') - (*b != '\0'); /* more identifiers rank higher */
        }
        ++a; /* skip the dots */
        ++b;
    }
}

int sota_version_compare(const sota_version_t *a, const sota_version_t *b) {
    int order = compare_numbers(a->major, b->major);
    if (order == 0) {
        order = compare_numbers(a->minor, b->minor);
    }
    if (order == 0) {
        order = compare_numbers(a->patch, b->patch);
    }
    if (order == 0) {
        order = compare_prerelease(a->prerelease, b->prerelease);
    }
    return order;
}

sota_decision_t sota_decide(const char *running_project, const char *running_version,
                            const char *candidate_project, const char *candidate_version) {
    if (strncmp(running_project, candidate_project, SOTA_PROJECT_NAME_SIZE) != 0) {
        return SOTA_OTHER_PROJECT;
    }
    sota_version_t running;
    sota_version_t candidate;
    if (!sota_version_parse(running_version, &running) || !sota_version_parse(candidate_version, &candidate)) {
        return SOTA_BAD_VERSION;
    }
    return sota_version_compare(&candidate, &running) > 0 ? SOTA_INSTALL : SOTA_NOT_NEWER;
}

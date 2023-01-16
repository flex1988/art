cc_library(
    name = "art",
    srcs = [
        "util.cpp",
        "adaptive_radix_tree.cpp",
    ],
    hdrs = [
        "util.h",
        "adaptive_radix_tree.h",
    ],
    deps = [
        "@com_github_gflags_gflags//:gflags"
    ]
)

cc_binary(
    name = "art_unittest",
    srcs = [
        "art_unittest.cpp",
    ],
    deps = [
        ":art",
        "@com_google_googletest//:gtest",
        "@com_github_gflags_gflags//:gflags"
    ]
)
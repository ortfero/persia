const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const test_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });
    test_mod.addCSourceFile(.{
        .file = b.path("test/test.cpp"),
        .flags = &.{
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            // doctest's single-header bundle calls deprecated `sprintf`; the
            // library code itself does not.
            "-Wno-deprecated-declarations",
        },
    });
    test_mod.addIncludePath(b.path("include"));
    test_mod.addIncludePath(b.path("test"));

    const test_exe = b.addExecutable(.{
        .name = "persia-test",
        .root_module = test_mod,
    });
    b.installArtifact(test_exe);

    b.installDirectory(.{
        .source_dir = b.path("include/persia"),
        .install_dir = .header,
        .install_subdir = "persia",
    });

    const run_tests = b.addRunArtifact(test_exe);
    if (b.args) |args| run_tests.addArgs(args);

    const test_step = b.step("test", "Build and run persia tests");
    test_step.dependOn(&run_tests.step);
}

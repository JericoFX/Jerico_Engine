import importlib, importlib.metadata as metadata, importlib.util, json, sys
expected_git = ("CPython", "cpython-3.10.13-archive", "49965601d6afedafe47cc85556d99b7a24981051")
expected_distributions = {"numpy": ("numpy", "numpy", "1.23.0"), "scipy": ("scipy", "scipy", "1.8.1"), "Pillow": ("PIL", "Pillow", "10.3.0"), "pytest": ("pytest", "pytest", "7.2.0"), "requests": ("requests", "requests", "2.32.0"), "GitPython": ("git", "GitPython", "3.1.44"), "o3de": ("o3de", "o3de", "1.0.0"), "ly-test-tools": ("ly_test_tools", "ly-test-tools", "1.0.0"), "ly-remote-console": ("ly_remote_console", "ly-remote-console", "1.0.0"), "editor-python-test-tools": ("editor_python_test_tools", "editor-python-test-tools", "1.0.0")}
results = {}
for key, (module_name, distribution, version) in expected_distributions.items():
    module = importlib.import_module(module_name)
    actual_version = metadata.version(distribution)
    results[key] = {"module": module_name, "module_file": getattr(module, "__file__", None), "distribution": distribution, "expected_version": version, "actual_version": actual_version, "version_match": actual_version == version, "spec_found": importlib.util.find_spec(module_name) is not None}
aws = {}
for name in ("boto3", "botocore", "s3transfer"):
    try:
        version = metadata.version(name)
        installed = True
    except metadata.PackageNotFoundError:
        version = None
        installed = False
    aws[name] = {"distribution_installed": installed, "module_spec_found": importlib.util.find_spec(name) is not None}
    if installed or aws[name]["module_spec_found"]:
        raise RuntimeError(f"AWS distribution/module unexpectedly present: {name}")
actual_git = tuple(getattr(sys, "_git", ()))
assert sys.version_info[:3] == (3, 10, 13), sys.version
assert actual_git == expected_git, actual_git
assert all(item["version_match"] and item["spec_found"] for item in results.values()), results
print(json.dumps({"sys_version": sys.version, "sys_git": actual_git, "pins_and_imports": results, "aws_absence": aws, "passed": True}, indent=2, default=str))

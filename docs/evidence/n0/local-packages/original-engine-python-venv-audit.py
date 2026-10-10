import importlib
import importlib.metadata as metadata
import importlib.util
import json
import sys

modules = ["numpy", "scipy", "PIL", "pytest", "requests", "git", "ly_test_tools", "ly_remote_console", "editor_python_test_tools", "o3de"]
module_results = {}
for name in modules:
    try:
        module = importlib.import_module(name)
        module_results[name] = {"ok": True, "file": getattr(module, "__file__", None), "version": getattr(module, "__version__", None)}
    except Exception as exc:
        module_results[name] = {"ok": False, "error": type(exc).__name__ + ": " + str(exc)}
aws = {}
for name in ["boto3", "botocore", "s3transfer"]:
    try:
        dist = metadata.distribution(name)
        aws[name] = {"installed": True, "version": dist.version}
    except metadata.PackageNotFoundError:
        aws[name] = {"installed": False, "module_spec": importlib.util.find_spec(name) is not None}
pyside = {}
for name in ["PySide6", "shiboken6", "PySide6_Addons", "PySide6_Essentials"]:
    try:
        pyside[name] = {"installed": True, "version": metadata.distribution(name).version}
    except metadata.PackageNotFoundError:
        pyside[name] = {"installed": False}
print(json.dumps({"executable": sys.executable, "version": sys.version, "prefix": sys.prefix, "base_prefix": sys.base_prefix, "modules": module_results, "aws_distributions": aws, "pyside_distributions": pyside, "pyside_import_performed": False}, indent=2))

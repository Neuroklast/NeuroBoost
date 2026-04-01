# Changelog

## v0.1.6-alpha

- Fix CI pipeline: VST3 SDK is now cloned directly from steinbergmedia/vst3sdk instead of via fragile git submodule init
- build.yml: fix wrong header path check, remove silent VST3 fallback, add post-build binary verification
- release.yml: remove `|| true` from SDK clone, add pre-release artifact count check

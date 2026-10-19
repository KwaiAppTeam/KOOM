# Contributing to KOOM

Welcome to the KOOM project and report Issues or pull requests.

## Issues

We use GitHub issues to track public bugs and feature requests.

### Search Known Issues First

Please search the existing issues to see if any similar issue or feature request has already been filed. You should make sure your issue isn't redundant.

### Reporting New Issues

If you open an issue, the more information the better. Such as detailed description, screenshot or video of your problem, logcat or code blocks for your crash.

### Pull Requests

Welcome your pull request.

## Branch Management

There is only one main branch:

* `master` branch

    * It's the latest (pre-)release branch. We use `master` for tags.
    * Feature or bugfix branch pull out from `master` and merge after everything is ok. 
    
## Pull Requests

Please make sure the following is done when submitting a pull request:

1. Fork the repo and create your feature or bugfix branch from `master`.
2. Add the copyright notice to the top of any new files you've added.
3. Try your best to test your code.
4. Squash all of your commits into one meaningful commit.


## Code Style Guide

1. For Java Code Style reference [AOSP Java Code Style](https://source.android.com/setup/contribute/code-style) except 2 spaces for indentation.
2. For C++ Code Style reference [Google C++ Code Style](https://google.github.io/styleguide/cppguide.html).


## Maven Central Publishing

Configure `sonatypeUsername` / `sonatypePassword` with a [Central Portal user token](https://central.sonatype.org/publish/generate-portal-token/)
in `~/.gradle/gradle.properties`, or set `ORG_GRADLE_PROJECT_sonatypeUsername` / `ORG_GRADLE_PROJECT_sonatypePassword`.
Keep the existing PGP signing configuration in `.gradle/gradle.properties` and set `VERSION_NAME` before building:

```sh
./gradlew assembleRelease :koom-common:third-party:shark:jar -x :koom-demo:assembleRelease
./gradlew publishToSonatype closeSonatypeStagingRepository
```

Closing transfers the deployment to the [Central Portal](https://central.sonatype.com/publishing/deployments) for validation and manual publication.
For `-SNAPSHOT` versions, run `publishToSonatype` without the close task.

## License

By contributing to KOOM, you agree that your contributions will be licensed under its [Apache LICENSE](./LICENSE).
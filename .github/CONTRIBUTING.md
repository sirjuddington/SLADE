## Submitting an Issue

### Bug Reports

When submitting a bug report, be sure to include the version of SLADE you are using, your OS and any other details that may be helpful.

If it is a crash, be sure to include the stack trace that was output when the program crashed, if it showed up. The crash dialog should look something like this:

<p align="center"><img src="http://i.imgur.com/NJvhqxw.png"/></p>

To get the stack trace, simply click the 'Copy Stack Trace' button.

### Feature Requests

If you have a feature request, first make sure that it hasn't already been [requested](https://github.com/sirjuddington/SLADE/issues?q=is%3Aopen+is%3Aissue+label%3A%22feature+request%22), and that it is not already a [planned feature](https://github.com/sirjuddington/SLADE/wiki/Planned-Features).

## Submitting a Pull Request

### Branches

Before submitting a pull request, make sure it is based off of the correct branch. Currently there are two main branches for SLADE development: **master** and **stable**.

The **stable** branch is for the current *stable* release of SLADE. As the code in this branch must remain as stable as possible, only submit a pull request for this branch if it is a bug fix or minor improvement that is unlikely to introduce any new bugs. If you have implemented a new feature in your fork, consider submitting it as a pull request for the master branch.

The **master** branch is for the *next* release of SLADE. This branch is where all the new features are developed, and it isn't guaranteed to be stable. If your pull request does not fit the criteria for submitting to the stable branch as above, this is where it should go.

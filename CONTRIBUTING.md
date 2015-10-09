## Submitting an Issue

When submitting a new issue, remember to select any appropriate labels for it (they can be set to the top-right of the issue description text box), such as `bug` or `feature request`.

#### Bug Reports

When submitting a bug report, be sure to include the version of SLADE you are using, your OS and any other details that may be helpful.

If it is a crash, be sure to include the stack trace that was output when the program crashed, if it showed up. The crash dialog should look something like this:

<p align="center"><img src="http://i.imgur.com/5YlnS2n.png"/></p>

To get the stack trace, simply copy all the text from the scrollable text box.

#### Feature Requests

If you have a feature request, first make sure that it hasn't already been [requested](https://github.com/sirjuddington/SLADE/issues?q=is%3Aopen+is%3Aissue+label%3A%22feature+request%22), and that it is not already a [planned feature](https://github.com/sirjuddington/SLADE/wiki/Planned-Features).

## Submitting a Pull Request

#### Branches

Before submitting a pull request, make sure it is based off of the correct branch. Currently there are two main branches for SLADE development: **master** and **3.x.x**.

The **master** branch is for the current *stable* release of SLADE. As the code in this branch must remain as stable as possible, only submit a pull request for this branch if it is a bug fix or minor improvement that is unlikely to introduce any new bugs. If you have implemented a new feature in your fork, consider submitting it as a pull request for the 3.x.x branch.

The **3.x.x** branch is for the *next* release of SLADE. The branch name will change depending on what the next version is (currently this is **3.1.1**). This branch is where all the new features are developed, and it isn't guaranteed to be stable. If your pull request does not fit the criteria for submitting to the master branch as above, this is where it should go.

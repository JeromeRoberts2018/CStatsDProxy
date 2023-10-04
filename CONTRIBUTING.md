# Contributing to CStatsDProxy

We welcome contributions to the CStatsDProxy project! This document outlines the process to help get your contribution accepted.

## Licensing and Rights

By submitting a contribution to this project, you agree to license your contribution under the same license as the main project (as found in the `LICENSE` file in the root of this repository).

## Code Standards

This project is written in C, and we follow the ANSI C standard as closely as possible. Please make sure your code adheres to these guidelines.

- Variables should be named clearly and be self-explanatory.
- Comments: Your code should be self-explanatory but you should also include comments explaining tricky or non-intuitive portions of code.

## How to Contribute

1. **Fork the Repository**: Start by forking the CStatsDProxy repository.

2. **Clone the Forked Repo**: Clone your fork locally on your machine.

    ```bash
    git clone https://github.com/JeromeRoberts2018/CStatsDProxy.git
    ```

3. **Create a New Branch**: Create a new branch to work on. We recommend naming the branch such that it's clear what it's for.

    ```bash
    git checkout -b new-feature-or-fix
    ```

4. **Make Changes**: Make your changes or additions in your local copy. Make sure your code adheres to the existing style.

5. **Commit Your Changes**: Once you have made all your changes, and they are to your satisfaction, commit them.

    ```bash
    git add .
    git commit -m "Description of changes"
    ```

6. **Push to Your Fork**: Push your changes back to your fork on GitHub.

    ```bash
    git push origin new-feature-or-fix
    ```

7. **Submit a Pull Request**: Go to your fork on GitHub and click the "New Pull Request" button, and follow the prompts to create a pull request.

## Testing

Before submitting your changes, make sure all existing tests pass. If you are introducing new functionality, please write new tests to cover it. Refer to the Makefile for running tests:

```bash
make tests

# GitHub Project Support

This repository includes a GitHub Actions workflow that can automatically add newly opened issues and pull requests to a GitHub Project.

## Files Added

- `.github/workflows/add-to-project.yml`
- `.github/ISSUE_TEMPLATE/bug_report.yml`
- `.github/ISSUE_TEMPLATE/feature_request.yml`
- `.github/PULL_REQUEST_TEMPLATE.md`

## One-Time Repository Setup

1. Create or choose a GitHub Project.
2. Copy the project URL.
3. In the repository settings, add an Actions variable named `PROJECT_URL`.
4. Create a Personal Access Token that can update the target project.
5. In the repository settings, add that token as an Actions secret named `ADD_TO_PROJECT_PAT`.

## Token Requirements

- If the project is a user-owned project, the token owner must be able to edit that project.
- If the project is an organization-owned project, the token owner must have project access in that organization.
- Store the token only in `ADD_TO_PROJECT_PAT`; the workflow reads it from the repository secret.

## Workflow Behavior

- New issues are added when they are opened or reopened.
- Pull requests are added when they are opened, reopened, or marked ready for review.
- If `PROJECT_URL` or `ADD_TO_PROJECT_PAT` is missing, the workflow is skipped instead of failing.

## Recommended Next Step

After the repository variable and secret are configured, open a test issue and a test pull request to confirm that cards appear in the target GitHub Project.

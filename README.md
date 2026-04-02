# StepDance Course Project Portfolio

This repository is your project portfolio for the Creative Motion Control course. You will use it to store code and documentation for each project you complete.

## Repository Structure

```
projects/
  project1/
    code/          ← Your Arduino/C++ code
    docs/
      index.md     ← Project write-up (text, images, video)
      assets/      ← Images and media filesgi
  project2/
    ...
```

## Getting Started

1. **Clone this repository** (or use the GitHub template to create your own copy).
2. **Enable GitHub Pages**: Go to your repo's *Settings → Pages*, set Source to **GitHub Actions**.
3. **Start a new project**: Duplicate the `projects/project1/` folder and rename it for each new project (e.g., `project2`, `project3`).
4. **Add your code**: Place your Arduino `.ino` and any supporting `.h`/`.cpp` files in the `code/` folder.
5. **Write your documentation**: Edit `docs/index.md` to describe your project, embed images and video.
6. **Update the home page**: Add a link to your new project in `index.md` at the repo root.
7. **Push to GitHub**: Your documentation site will automatically deploy to `https://<your-username>.github.io/<repo-name>/`.

## Writing Documentation

Each project's `docs/index.md` is a Markdown file that supports:

- **Images**: Place files in `docs/assets/` and reference them:
  ```markdown
  ![Description](assets/my-image.jpg)
  ```
- **Embedded video** (YouTube, Vimeo, etc.):
  ```html
  <iframe width="560" height="315" src="https://www.youtube.com/embed/VIDEO_ID" frameborder="0" allowfullscreen></iframe>
  ```
- **Local video files**:
  ```html
  <video width="560" controls>
    <source src="assets/my-video.mp4" type="video/mp4">
  </video>
  ```

## Resources

- [StepDance Library Documentation](https://stepdance.org)
- [Arduino Language Reference](https://www.arduino.cc/reference/en/)
- [Markdown Guide](https://www.markdownguide.org/)
- [GitHub Pages Documentation](https://docs.github.com/en/pages)
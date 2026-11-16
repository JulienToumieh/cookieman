# CookieMan
A simple utility program for creating base projects from templates.

## Features
- Looks in all the file names and contents in the passed directory for fields of the format "{{ text }}" where the text can contain any characters
- Replaces all the instances of the fields with the required names the user provides

This allows project templates to be easily customized and ready in seconds.

## Usage
- Add CookieMan to the path
- Call `cookieman [project directory]`
- Replace the detected fields with the desired names

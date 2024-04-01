# CHANGELOG - IPK Project 1: Client for chat server using `IPK24-CHAT` protocol

All changes in the project can be found in the project repository's [commit history](https://git.fit.vutbr.cz/xvalik05/IPK_project1/commits/branch/main), which will be summarized below.


## Notable Changes

### March 4, 2024
- Implemented argument parsing and initial object-oriented design to establish the project's structure.
- Added Client's constructor, which creates a socket, resolves hostname, and prepares server address.

### March 17, 2024
- Finalized object-oriented design.
- Implemented the main loop, which is generalized for both variants (TCP and UDP).
- Implemented the UDP variant of the client.

### March 19, 2024
- Added TCP implementation for the client.
- Added program documentation (doxygen comments).

### March 28, 2024
- Implemented fixes to enhance functionality.

### April 1, 2024
- Added project documentation and finalized the project.

## Known Limitations
- UDP confirmation: Incoming messages have the potential to extend the confirmation timeout theoretically. This occurs because the timeout is set on the socket and cannot ignore other messages.

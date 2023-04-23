# Terms
- Sign XXX = Sign the digest of XXX

# Exaustive List of Cryptographic Operations
- Clients should **encrypt** the payload using per-file symmetric key before passing it to the middleware.
- Clients should **sign** the arguments using their own private key and include it to the request to the middleware.
- Middleware should **verify** the arguments using the public key.
- Middleware should **sign** DataCapsule header and include the signature of it in DataCapsule before sending it to DCServer.
- Clients should **decrypt** the payload using per-file symmetric key after they get the records from DCServer.

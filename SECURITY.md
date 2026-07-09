# Security Policy

## Supported Versions

Security fixes are targeted at the default branch.

## Reporting a Vulnerability

Please report vulnerabilities privately through GitHub Security Advisories when available, or contact the repository owner directly. Do not open public issues for exploitable bugs, leaked credentials, or sensitive operational details.

For reports, include:

- affected commit, tag, or release
- clear reproduction steps
- expected impact
- suggested mitigation, if known

## Secret Handling

Do not commit private RPC endpoints, API keys, wallet keys, mnemonics, private keys, or production `NeoZorK-config` files. Keep local configuration out of git and use `NeoZorK-config.example` for safe examples.

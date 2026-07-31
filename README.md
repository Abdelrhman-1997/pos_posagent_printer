# POSAgent Printer for Odoo 18

Free local USB printing bridge for Odoo 18 POS.

## Current status

- The existing `pos_posagent` addon was migrated in place to Odoo 18.
- Odoo core is not modified.
- The Windows agent is based on the MIT-licensed upstream project `dieg0-a/posagentpro-src`.
- The baseline Windows build is pinned to upstream commit `fcbdae2458111f1ad5d52f2f4ebe90fd7b3d8bc2`.
- Multi-printer routing for cashier, kitchen, and coffee is not implemented yet.

## Architecture

```text
Odoo 18 server
    -> POS browser on the customer's Windows PC
    -> http://127.0.0.1:9069
    -> POSAgent tray application
    -> local USB printer through the Windows driver
```

Because printing is sent through the local agent rather than `window.print()`, the browser print dialog is bypassed.

## Repository layout

- `pos_posagent/`: Odoo 18 addon.
- `scripts/build_windows.ps1`: local Windows baseline build helper.
- `.github/workflows/build-windows.yml`: reproducible Windows x64 CI build.
- `UPSTREAM_COMMIT`: exact upstream source revision used by the build.

The CI workflow fetches the pinned upstream source during the build. This keeps the first baseline commit small and reproducible. After the baseline artifact is verified on a real Windows PC, the C++ source can be imported and customized for multiple printers.

## Build artifact

GitHub Actions produces an artifact named `posagent-win64-dev` containing:

- `posagent-win64-dev.zip`
- `SHA256SUMS.txt`
- `build-summary.txt`
- configure/build/deployment logs

The ZIP includes the executable and required Qt runtime files. It is not an installer yet.

## Required real-device test

1. Install a USB thermal printer using its Windows driver.
2. Confirm a Windows test page prints.
3. Download and extract `posagent-win64-dev.zip`.
4. Run the agent and choose the printer.
5. Enable POSAgent in the Odoo POS configuration and use the same local port.
6. Validate an order and confirm direct printing without the browser dialog.

## Licensing

- Windows POSAgent upstream source: MIT License, copyright Diego A. The build bundles the upstream license notice.
- Odoo addon: LGPL-3, preserving the original module licensing and attribution.

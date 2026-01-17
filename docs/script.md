## Yes: we started theoretical planning on the VM management script

The shape we were heading toward was basically a tiny command language / REPL that can be driven both interactively and via a script file (like your ARM-VM’s `do` command idea), with commands like:

- `vm create <name>`

- `vm use <id>`

- `set ram <bytes>`

- `set cpu <type>` (later)

- `add disk <path> [--type=ide] [--readonly]`

- `add rom <path> [--addr=...]`

- `add nic ...` (later)

- `boot <addr | bios>`

- `run [--steps=N]`

- `step [N]`

- `regs`

- `mem <addr> <len>`

- set cpu debug=all

- load mov_ax_imm.bin 0000:1000

- set CS 0
  set IP 0x1000

- logfile mov_ax_imm.log

  list vm

  -----------------------------

  ## The big design choice: who “owns” what?

  ### Ownership (recommended)

  - `main()` creates **Session**
  - Session creates/owns **VMManager**
  - VMManager creates/owns **VM instances**
  - CLI operates on Session (and therefore on VMs *through* the manager)

  So you get:

  ```
  main
   └─ Session
       ├─ Vars (key/value)
       ├─ Output policy (console/log tee)
       ├─ VMManager
       │    ├─ VM #1
       │    ├─ VM #2
       │    └─ ...
       └─ current_vmid
  ```

  This maps exactly to your “VMID=1” idea.

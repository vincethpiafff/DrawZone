oscPort: Oil.PositiveInteger(9000)
oscListen: Oil.Boolean(True)
oscClear: Oil.Boolean(False)
oscStatus: Oil.String("")

# DrawZone OSC bank - SMODE Python script
#
# Receives OSC messages (from DrawZone / Chataigne or any OSC sender) and shows
# every OSC address as a parameter of a Parameter Bank, updated in real time.
#
# Setup in SMODE:
#   1. Add a Python Script in a Scene or a Compo (its tools), paste this file.
#   2. Launch Mode = At Every Update.
#   3. Set Osc Port to the port DrawZone sends to (default 9000). Use a port
#      that no SMODE OSC device listens to.
#   The parameters appear in the Parameter Bank of the same Scene / Compo
#   (created if there is none), one per OSC address, labelled with the address.
#
# OSC types: f d -> Real, i h -> Integer, T F -> Boolean, s -> String.
# A message with several values gives one parameter per value: "/addr [1]", "/addr [2]".
#
# Osc Clear: removes the parameters created by this script (labels starting with "/").
#
# The SMODE Python API is not published: this script follows the unofficial
# reference https://github.com/gyomh/smode-oil-reference (configure an object
# before appending it, keep per-script state out of shared globals, never let
# an exception escape an At Every Update script).


def _dzosc_main():
    import socket
    import struct
    import time

    MAX_NEW_PER_FRAME = 20      # parameters created per frame, so a burst of addresses does not freeze SMODE
    MAX_PARAMETERS = 500
    MAX_PACKETS_PER_FRAME = 2000

    # ---- per-script state (scripts share their globals in SMODE) ----
    reg = globals().setdefault("_DZOSC_REGISTRY", [])
    st = None
    for owner, d in reg:
        if owner is script:
            st = d
            break
    if st is None:
        st = {}
        reg.append((script, st))

    def set_status(text):
        if st.get("status") != text:
            st["status"] = text
            script.oscStatus.set(text)

    # ---- OSC parsing (stdlib only) ----
    def read_string(data, i):
        j = data.index(b"\0", i)
        return data[i:j].decode("utf-8", "replace"), (j + 4) & ~3

    def parse_message(data, out):
        address, i = read_string(data, 0)
        if not address.startswith("/"):
            return
        tags = ""
        if i < len(data) and data[i:i + 1] == b",":
            tags, i = read_string(data, i)
            tags = tags[1:]
        values = []
        for t in tags:
            if t == "f":
                values.append(("Real", struct.unpack(">f", data[i:i + 4])[0])); i += 4
            elif t == "i":
                values.append(("Integer", struct.unpack(">i", data[i:i + 4])[0])); i += 4
            elif t == "d":
                values.append(("Real", struct.unpack(">d", data[i:i + 8])[0])); i += 8
            elif t == "h":
                values.append(("Integer", struct.unpack(">q", data[i:i + 8])[0])); i += 8
            elif t == "s" or t == "S":
                s, i = read_string(data, i)
                values.append(("String", s))
            elif t == "T":
                values.append(("Boolean", True))
            elif t == "F":
                values.append(("Boolean", False))
            elif t == "b":
                n = struct.unpack(">i", data[i:i + 4])[0]
                i += 4 + ((n + 3) & ~3)
            elif t in "NI":
                pass
            elif t in "crm":
                i += 4
            elif t == "t":
                i += 8
            else:
                return  # unknown type: the rest cannot be read
        if len(values) == 1:
            out[address] = values[0]
        else:
            for k, v in enumerate(values):
                out[address + " [" + str(k + 1) + "]"] = v

    def parse_packet(data, out):
        if data.startswith(b"#bundle\0"):
            i = 16  # "#bundle\0" + time tag
            while i + 4 <= len(data):
                n = struct.unpack(">i", data[i:i + 4])[0]
                parse_packet(data[i + 4:i + 4 + n], out)
                i += 4 + n
        elif data.startswith(b"/"):
            parse_message(data, out)

    # ---- socket ----
    def close_socket():
        sock = st.pop("sock", None)
        if sock is not None:
            try:
                sock.close()
            except Exception:
                pass
        st["port"] = None

    port = int(script.oscPort.get())
    if not script.oscListen.get():
        close_socket()
        set_status("Stopped")
        return

    if st.get("sock") is None or st.get("port") != port:
        close_socket()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind(("0.0.0.0", port))
            sock.setblocking(False)
            st["sock"] = sock
            st["port"] = port
        except Exception as e:
            set_status("Cannot listen on port " + str(port) + ": " + str(e))
            return

    # ---- parameter bank ----
    def get_bank():
        parent = script.parentElement
        tools = parent.tools
        for k in range(len(tools)):
            t = tools[k]
            if t.getOilClassName() == "ParameterBank":
                return t
        bank = Oil.createObject("ParameterBank")
        tools.append(bank)
        return None  # appended this frame: read it back on the next one

    bank = get_bank()
    if bank is None:
        set_status("Parameter Bank created")
        return

    def label_of(p):
        try:
            return str(p.label.get())
        except Exception:
            return str(p.label)

    pending = st.setdefault("pending", {})   # label -> [type, frames], appended, not read back yet
    kinds = st.setdefault("kinds", {})       # label -> Oil class of its parameter
    last = st.setdefault("last", {})         # label -> last value written

    if script.oscClear.get():
        script.oscClear.set(False)
        plist = bank.parameters
        for k in range(len(plist) - 1, -1, -1):
            if label_of(plist[k]).startswith("/"):
                plist.removeAt(k)
        pending.clear(); kinds.clear(); last.clear()
        set_status("Cleared")
        return

    # fresh references every frame: a parameter deleted by hand in SMODE leaves no dangling wrapper
    params = {}  # label -> (Oil parameter, Oil class)
    plist = bank.parameters
    for k in range(len(plist)):
        p = plist[k]
        lab = label_of(p)
        if not lab.startswith("/") or lab in params:
            continue
        if lab not in kinds:
            kinds[lab] = pending[lab][0] if lab in pending else p.getOilClassName()
        params[lab] = (p, kinds[lab])
        pending.pop(lab, None)
    for lab in list(pending):
        pending[lab][1] += 1
        if pending[lab][1] > 120:
            del pending[lab]  # never showed up in the bank: try again on its next message
    for lab in list(last):
        if lab not in params and lab not in pending:
            del last[lab]  # deleted by hand: created again on its next message
            kinds.pop(lab, None)

    # ---- read every packet waiting on the socket ----
    received = {}
    sock = st["sock"]
    for _ in range(MAX_PACKETS_PER_FRAME):
        try:
            data, _addr = sock.recvfrom(65536)
        except BlockingIOError:
            break
        except OSError:
            break  # Windows: ICMP "port unreachable" from an earlier send, ignore
        try:
            parse_packet(data, received)
        except Exception:
            st["badPackets"] = st.get("badPackets", 0) + 1
    if received:
        st["lastReceive"] = time.time()
        st["messages"] = st.get("messages", 0) + len(received)

    # ---- create the missing parameters, write the new values ----
    TYPE_CANDIDATES = {
        "Real": ("Parameter(Real)", "Parameter(UnboundedPercentage)"),
        "Integer": ("Parameter(Integer)", "Parameter(Real)"),
        "Boolean": ("Parameter(Boolean)",),
        "String": ("Parameter(String)",),
    }
    created = 0
    for lab, (kind, value) in received.items():
        if lab in params:
            p, pkind = params[lab]
            if last.get(lab) == value:
                continue
            try:
                if kind == "String":
                    p.value.set(str(value))
                elif kind == "Boolean":
                    p.value.set(bool(value))
                elif "Integer" in pkind and kind == "Integer":
                    p.value.set(int(value))
                else:
                    p.value.set(float(value))
                last[lab] = value
            except Exception as e:
                st["setErrors"] = st.get("setErrors", 0) + 1
                st["lastError"] = lab + ": " + str(e)
            continue

        if lab in pending or created >= MAX_NEW_PER_FRAME or len(params) + len(pending) >= MAX_PARAMETERS:
            continue

        # golden rule: configure everything, append last, never touch the object afterwards
        for type_name in TYPE_CANDIDATES[kind]:
            try:
                p = Oil.createObject(type_name)
            except Exception:
                p = None
            if p is None:
                continue
            try:
                p.label = lab
                if kind == "String":
                    p.value.set(str(value))
                elif kind == "Boolean":
                    p.value.set(bool(value))
                elif type_name == "Parameter(Integer)":
                    p.value.set(int(value))
                else:
                    p.value.set(float(value))
            except Exception:
                pass
            bank.parameters.append(p)
            pending[lab] = [type_name, 0]
            last[lab] = value
            created += 1
            break
        else:
            st["unsupported"] = st.get("unsupported", 0) + 1

    # ---- status ----
    ago = time.time() - st.get("lastReceive", 0)
    text = "Port " + str(port) + " - " + str(len(params)) + " parameters"
    text += " - receiving" if ago < 1.0 else " - waiting for OSC"
    if st.get("setErrors"):
        text += " - " + str(st["setErrors"]) + " errors (last: " + st.get("lastError", "") + ")"
    if st.get("unsupported"):
        text += " - " + str(st["unsupported"]) + " unsupported"
    if len(params) + len(pending) >= MAX_PARAMETERS:
        text += " - limit of " + str(MAX_PARAMETERS) + " reached"
    set_status(text)


try:
    _dzosc_main()
except Exception as _dzosc_error:
    try:
        script.oscStatus.set("Error: " + str(_dzosc_error))
    except Exception:
        pass

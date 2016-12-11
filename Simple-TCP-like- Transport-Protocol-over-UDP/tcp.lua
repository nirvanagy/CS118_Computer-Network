tcp = Proto("cs118tcp", "CS118 TCP")

local f_seqno  = ProtoField.uint16("cs118tcp.seqno", "Sequence Number")
local f_ack    = ProtoField.uint16("cs118tcp.ack", "ACK Number")
local f_window = ProtoField.uint16("cs118tcp.window", "Window")
local f_flags  = ProtoField.uint16("cs118tcp.flags", "Flags")

tcp.fields = { f_seqno, f_ack, f_window, f_flags }

function tcp.dissector(tvb, pInfo, root) -- Tvb, Pinfo, TreeItem
   if (tvb:len() ~= tvb:reported_len()) then
      return 0 -- ignore partially captured packets
      -- this can/may be re-enabled only for unfragmented UDP packets
   end

   local t = root:add(tcp, tvb(0,8))
   t:add(f_seqno, tvb(0,2))
   t:add(f_ack, tvb(2,2))
   t:add(f_window, tvb(4,2))
   local f = t:add(f_flags, tvb(6,2))

   local flag = tvb(7,1):uint()

   if bit.band(flag, 1) ~= 0 then
      f:add(tvb(7,2), "FIN")
   end
   if bit.band(flag, 2) ~= 0 then
      f:add(tvb(7,2), "SYN")
   end
   if bit.band(flag, 4) ~= 0 then
      f:add(tvb(7,2), "ACK")
   end

   local flag = tvb(6,1):uint()
   if bit.band(flag, 1) ~= 0 then
      f:add(tvb(6,1), "xFIN")
   end
   if bit.band(flag, 2) ~= 0 then
      f:add(tvb(6,1), "xSYN")
   end
   if bit.band(flag, 4) ~= 0 then
      f:add(tvb(6,1), "xACK")
   end
   
   pInfo.cols.protocol = "TCP CS118"
end

local udpDissectorTable = DissectorTable.get("udp.port")
udpDissectorTable:add("4000", tcp)

io.stderr:write("tcp.lua is successfully loaded\n")


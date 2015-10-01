import gtk
import math
import goocanvas
import pango
import cairo
import ns.core
import ns.network
import ns.visualizer
import ns.pmtmgmp

from visualizer.base import Link, transform_distance_canvas_to_simulation

SHOW_LOG = True
COLOR = [0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF, 0xFF00FFFF, 0xFFFF00FF, 0x8A2BE2FF, 0xB8860BFF, 0x006400FF,
         0xFF1493FF]
POINT_MODIFY = 10
SPEED_UPDATE_TIME = 1000
PI_OVER_2 = math.pi / 2
PI_TIMES_2 = math.pi * 2


class Pmtmgmp_Path_Link(Link):
    def __init__(self, next_link, base_node, link_node, base_route_table, mterp, msecp, parent_canvas_item, color, path,
                 viz, data_link, link_dict, link_key):
        self.base_node = base_node
        self.link_node = link_node
        self.base_route_table = base_route_table
        self.mterp = mterp
        self.msecp = msecp
        self.canvas_item = goocanvas.Group(parent=parent_canvas_item)
        if data_link:
            # self.line = goocanvas.Polyline(parent=self.canvas_item, line_width=1.0, stroke_color_rgba=0xC00000FF, start_arrow=True, arrow_length=10,arrow_width=8)
            self.line = goocanvas.Polyline(parent=self.canvas_item, line_width=3.0, stroke_color_rgba=0xC00000FF,
                                           start_arrow=True, close_path=False, end_arrow=False, arrow_length=20,
                                           arrow_width=25)
        else:
            # self.line = goocanvas.Polyline(parent=self.canvas_item, line_width=1.0, stroke_color_rgba=0xC00000FF, start_arrow=True, arrow_length=10,arrow_width=8)
            self.line = goocanvas.Polyline(parent=self.canvas_item, line_width=2.0, stroke_color_rgba=color,
                                           start_arrow=True, close_path=False, end_arrow=False, arrow_length=25,
                                           arrow_width=20)
        self.line.raise_(None)
        self.label = goocanvas.Text()  # , fill_color_rgba=0x00C000C0)
        self.label.props.pointer_events = 0
        self.label.set_property("parent", self.canvas_item)
        self.label.raise_(None)
        self.canvas_item.set_data("pyviz-object", self)
        self.canvas_item.lower(None)
        self.way_index = 0
        self.next_link = next_link
        self.path = path
        self.viz = viz
        self.data_link = data_link
        self.link_dict = link_dict
        self.link_key = link_key

    def set_way_index(self, index):
        self.way_index = index

    def set_next_link(self, link):
        next_link = link

    def set_color(self, color):
        self.line.set_properties(stroke_color_rgba=color)

    def move_node_and_lable(self):
        pos1_x, pos1_y = self.link_node.viz_node.get_position()
        pos2_x, pos2_y = self.base_node.viz_node.get_position()
        angle = math.atan2(pos2_y - pos1_y, pos2_x - pos1_x) + math.pi / 2
        x = POINT_MODIFY * math.cos(angle)
        y = POINT_MODIFY * math.sin(angle)
        if (self.way_index != 0):
            next_index = 0
            if self.next_link != None:
                next_index = self.next_link.way_index
            pos1_x = pos1_x + x * next_index
            pos1_y = pos1_y + y * next_index
            pos2_x = pos2_x + x * self.way_index
            pos2_y = pos2_y + y * self.way_index
        self.line.set_property("points", goocanvas.Points([(pos1_x, pos1_y), (pos2_x, pos2_y)]))
        p1_x, p1_y = self.link_node.viz_node.get_position()
        p2_x, p2_y = self.base_node.viz_node.get_position()
        if -PI_OVER_2 <= angle <= PI_OVER_2:
            if self.data_link:
                send_to = self.link_node.pmtmgmp.GetMacAddress()
                pmtmgmp = self.base_node.pmtmgmp
                if (ns.core.Simulator.Now().GetMilliSeconds() - self.base_node.data_speed_time) == 0:
                    self.destroy()
                    return
                else:
                    speed = (float)(pmtmgmp.GetPacketSizePerPathSecondByMac(send_to) - self.base_node.packet_size_map[
                        self.link_key]) * 8 * 1000 / SPEED_UPDATE_TIME / (
                            ns.core.Simulator.Now().GetMilliSeconds() - self.base_node.data_speed_time)
                    if speed == 0:
                        self.destroy()
                        return
                self.label.set_properties(font=("Sans Serif %i" % int(3 + self.viz.node_size_adjustment.value * 4)),
                                          text=("%.2f kbit/s" % (speed,)),
                                          alignment=pango.ALIGN_CENTER,
                                          x=x, y=-0.5 + y)
            else:
                self.label.set_properties(font=("Sans Serif %i" % int(1 + self.viz.node_size_adjustment.value * 4)),
                                          text=("%d" % (self.path.GetMetric(),)),
                                          alignment=pango.ALIGN_CENTER,
                                          x=x, y=-0.5 - y)
            M = cairo.Matrix()
            M.translate(*self.viz._get_label_over_line_position(p1_x, p1_y, p2_x, p2_y))
            M.rotate(angle)
        else:
            if self.data_link:
                send_to = self.link_node.pmtmgmp.GetMacAddress()
                pmtmgmp = self.base_node.pmtmgmp
                if (ns.core.Simulator.Now().GetMilliSeconds() - self.base_node.data_speed_time) == 0:
                    speed = 0
                else:
                    speed = (float)(pmtmgmp.GetPacketSizePerPathSecondByMac(send_to) - self.base_node.packet_size_map[
                        self.link_key]) * 8 * 1000 / SPEED_UPDATE_TIME / (
                            ns.core.Simulator.Now().GetMilliSeconds() - self.base_node.data_speed_time)
                self.label.set_properties(font=("Sans Serif %i" % int(3 + self.viz.node_size_adjustment.value * 3)),
                                          text=("%.2f kbit/s" % (speed,)),
                                          alignment=pango.ALIGN_CENTER,
                                          x=0, y=0.5)
            else:
                self.label.set_properties(font=("Sans Serif %i" % int(1 + self.viz.node_size_adjustment.value * 3)),
                                          text=("%d" % (self.path.GetMetric(),)),
                                          alignment=pango.ALIGN_CENTER,
                                          x=0, y=0.5)
            M = cairo.Matrix()
            M.translate(*self.viz._get_label_over_line_position(p1_x, p1_y, p2_x, p2_y))
            M.rotate(angle)
            M.scale(-1, -1)
        self.label.set_transform(M)

    def update_points(self):
        if self.data_link:
            if (self.base_node == None) or (self.link_node == None):
                # if SHOW_LOG:
                #     print "Delete(MAC) " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
                self.destroy()
                return
            send_to = self.link_node.pmtmgmp.GetMacAddress()
            pmtmgmp = self.base_node.pmtmgmp
            if (not self.base_node.packet_size_map.has_key(self.link_key)) or (
                    pmtmgmp.GetPacketSizePerPathSecondByMac(send_to) - self.base_node.packet_size_map[
                    self.link_key] == 0) and (self.base_node.data_speed_time_temp != self.base_node.data_speed_time):
                # if SHOW_LOG:
                #     if (self.base_node.packet_size_map.has_key(self.link_key)):
                #         print "Delete(Speed) " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(
                #             self.link_node.pmtmgmp.GetMacAddress()) + "pmtmgmp:" + str(
                #             pmtmgmp.GetPacketSizePerPathSecondByMac(send_to)) + ",record" + str(
                #             self.base_node.packet_size_map[self.link_key])
                self.destroy()
            else:
                # if SHOW_LOG:
                #     print "Move " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
                self.move_node_and_lable()
        else:
            if self.base_route_table.GetPathByMACaddress(self.mterp, self.msecp) == None:
                # if SHOW_LOG:
                #     print "Delete(Path) " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
                self.destroy()
            elif (self.base_node == None) or (self.link_node == None):
                # if SHOW_LOG:
                #     print "Delete(MAC) " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
                self.destroy()
            else:
                # if SHOW_LOG:
                #     print "Move " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
                self.move_node_and_lable()

    def destroy(self):
        self.link_dict.pop(self.link_key)
        self.line.remove()
        self.label.remove()
        self.canvas_item.remove()
        self.ns_node = None
        self.pmtmgmp_node = None
        del (self)


class Pmtmgmp_Node(object):
    def __init__(self, node_index, viz_node, pmtmgmp, viz):
        self.node_index = node_index
        self.viz_node = viz_node
        self.base_mac = pmtmgmp.GetMacAddress()
        self.base_route_table = pmtmgmp.GetPmtmgmpRouteTable()
        self.pmtmgmp = pmtmgmp
        self.link_dict = {}
        self.viz = viz
        self.packet_size_map = {}
        self.packet_size_map_temp = {}
        self.data_speed_time = 0
        self.data_speed_time_temp = 0
        self.packet_size_map_first = True

    def set_color(self, color):
        self.viz_node.canvas_item.set_properties(fill_color_rgba=color)
        # if SHOW_LOG:
        #     print str(self.pmtmgmp.GetMacAddress()) + " set color " + str(color)

    def clean_link_dict(self):
        for (key, link) in self.link_dict.items():
            link.destroy()
            if self.link_dict.has_key(key):
                del (self.link_dict[key])
        self.link_dict.clear()
        self.link_dict = {}

    def update_link_dict(self, type):
        if type and self.packet_size_map_first:
            self.packet_size_map_first = False
            self.data_speed_time_temp = ns.core.Simulator.Now().GetMilliSeconds()
            self.packet_size_map_temp = {}
            for i in range(0, self.pmtmgmp.GetPacketSizePerPathCount()):
                self.packet_size_map_temp[
                    str(self.pmtmgmp.GetPacketSizePerPathFirst(i))] = self.pmtmgmp.GetPacketSizePerPathSecond(i)
        if type and ns.core.Simulator.Now().GetMilliSeconds() - self.data_speed_time > SPEED_UPDATE_TIME:
            self.data_speed_time = self.data_speed_time_temp
            self.data_speed_time_temp = ns.core.Simulator.Now().GetMilliSeconds()
            self.packet_size_map = self.packet_size_map_temp.copy()
            for i in range(0, self.pmtmgmp.GetPacketSizePerPathCount()):
                self.packet_size_map_temp[
                    str(self.pmtmgmp.GetPacketSizePerPathFirst(i))] = self.pmtmgmp.GetPacketSizePerPathSecond(i)
        i = 0
        for (key, link) in self.link_dict.items():
            link.set_way_index(int((i + 1) / 2) * int((i % 2) * 2 - 1))
            i += 1
            # if SHOW_LOG:
            #     print "update_points() "  + str(link.base_node.pmtmgmp.GetMacAddress()) + " to " + str(link.link_node.pmtmgmp.GetMacAddress())
            link.update_points()

    def crearte_route_path(self, mac_node_list, mterp, msecp, parent_canvas_item):
        # if SHOW_LOG:
        #     print self

        node_type = self.pmtmgmp.GetNodeType()
        color = 0xFF0000FF
        if (node_type == 2):
            color = 0x00FF00FF
        elif (node_type == 4):
            color = 0x0000FFFF
        elif (node_type == 8 and msecp == self.pmtmgmp.GetMacAddress()):
            color = 0xFFFF00FF
        if (self.pmtmgmp.GetPmtmgmpRouteTable().GetTableSize() == 0):
            color = 0xFFFFFFFF
        self.set_color(color)

        route_table = self.pmtmgmp.GetPmtmgmpRouteTable()
        route_path = route_table.GetPathByMACaddress(mterp, msecp)
        key = str(mterp) + "," + str(msecp)
        if route_path == None:
            return
        if self.link_dict.has_key(key):
            self.link_dict[key].update_points()
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::crearte_route_path() exist " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        else:
            if mac_node_list[str(route_path.GetFromNode())] is self:
                return
            route_link = Pmtmgmp_Path_Link(None, self, mac_node_list[str(route_path.GetFromNode())], route_table, mterp,
                                           msecp, parent_canvas_item, COLOR[2], route_path, self.viz, False,
                                           self.link_dict, key)
            self.link_dict[key] = route_link
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::crearte_route_path() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        self.update_link_dict(False)

    def create_part_route_path_recursion(self, mac_node_list, mterp, msecp, parent_canvas_item, color):
        node_type = self.pmtmgmp.GetNodeType()
        node_color = 0xFF0000FF
        if (node_type == 2):
            node_color = 0x00FF00FF
        elif (node_type == 4):
            node_color = 0x0000FFFF
        elif (node_type == 8):
            node_color = 0xFFFF00FF
        if (self.pmtmgmp.GetPmtmgmpRouteTable().GetTableSize() == 0):
            node_color = 0xFFFFFFFF
        self.set_color(node_color)
        route_table = self.pmtmgmp.GetPmtmgmpRouteTable()
        route_path = route_table.GetPathByMACaddress(mterp, msecp)
        if route_path == None:
            return None
        next_hop = mac_node_list[str(route_path.GetFromNode())]
        if next_hop is self:
            return None
        next_link = next_hop.create_part_route_path_recursion(mac_node_list, mterp, msecp, parent_canvas_item, color)
        key = str(mterp) + "," + str(msecp)
        if self.link_dict.has_key(key):
            self.link_dict[key].set_color(color)
            self.link_dict[key].set_next_link(next_link)
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::create_part_route_path_recursion() exist " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        else:
            self.link_dict[key] = Pmtmgmp_Path_Link(next_link, self, mac_node_list[str(route_path.GetFromNode())],
                                                    route_table, mterp, msecp, parent_canvas_item, color, route_path,
                                                    self.viz, False, self.link_dict, key)
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::create_part_route_path_recursion() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        self.update_link_dict(False)
        return self.link_dict[key]

    def create_part_route_path_start(self, mac_node_list, mterp, parent_canvas_item):
        node_type = self.pmtmgmp.GetNodeType()
        node_color = 0xFF0000FF
        if (node_type == 2):
            node_color = 0x00FF00FF
        elif (node_type == 4):
            node_color = 0x0000FFFF
        elif (node_type == 8):
            node_color = 0xFFFF00FF
        if (self.pmtmgmp.GetPmtmgmpRouteTable().GetTableSize() == 0):
            node_color = 0xFFFFFFFF
        self.set_color(node_color)
        route_table = self.pmtmgmp.GetPmtmgmpRouteTable()
        route_tree = route_table.GetTreeByMACaddress(mterp)
        if route_tree == None:
            return
        for i in range(0, route_tree.GetTreeSize()):
            route_path = route_tree.GetTreeItem(i)
            msecp = route_path.GetMSECPaddress()
            next_hop = mac_node_list[str(route_path.GetFromNode())]
            if next_hop is self:
                return None
            next_link = next_hop.create_part_route_path_recursion(mac_node_list, mterp, msecp, parent_canvas_item,
                                                                  COLOR[i % 10])
            key = str(mterp) + "," + str(msecp)
            if self.link_dict.has_key(key):
                self.link_dict[key].set_color(COLOR[i % 10])
                self.link_dict[key].set_next_link(next_link)
                # if SHOW_LOG:
                #     print "Pmtmgmp_Node::create_part_route_path_recursion() exist " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
            else:
                self.link_dict[key] = Pmtmgmp_Path_Link(next_link, self, mac_node_list[str(route_path.GetFromNode())],
                                                        route_table, mterp, msecp, parent_canvas_item, COLOR[i % 10],
                                                        route_path, self.viz, False, self.link_dict, key)
                # if SHOW_LOG:
                #     print "Pmtmgmp_Node::create_part_route_path_recursion() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
            self.update_link_dict(False)

    def crearte_data_path(self, mac_node_list, mterp, msecp, parent_canvas_item):
        node_type = self.pmtmgmp.GetNodeType()
        color = 0xFF0000FF
        if (node_type == 2):
            color = 0x00FF00FF
        elif (node_type == 4):
            color = 0x0000FFFF
        elif (node_type == 8):
            color = 0xFFFF00FF
        if (self.pmtmgmp.GetPmtmgmpRouteTable().GetTableSize() == 0):
            color = 0xFFFFFFFF
        self.set_color(color)

        for i in range(0, self.pmtmgmp.GetPacketSizePerPathCount()):
            send_to = self.pmtmgmp.GetPacketSizePerPathFirst(i)
            key = str(send_to)
            able_create = True
            if ns.core.Simulator.Now().GetMilliSeconds() != self.data_speed_time and self.packet_size_map.has_key(key):
                speed = (float)(self.pmtmgmp.GetPacketSizePerPathSecondByMac(send_to) - self.packet_size_map[key]) * 8 * \
                    1000/ SPEED_UPDATE_TIME /(ns.core.Simulator.Now().GetMilliSeconds() - self.data_speed_time)
                if speed == 0:
                    able_create = False
            if able_create and ns.core.Simulator.Now().GetMilliSeconds() != self.data_speed_time and (not self.link_dict.has_key(str(send_to))):
                route_link = Pmtmgmp_Path_Link(None, self,
                                               mac_node_list[key], None,
                                               None, None, parent_canvas_item, None, None, self.viz, True,
                                               self.link_dict, key)
                self.link_dict[str(send_to)] = route_link
                # if SHOW_LOG:
                #     print "Pmtmgmp_Node::crearte_data_path() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(send_to)
        self.update_link_dict(True)

    def destory(self):
        self.node_index = None
        self.viz_node = None
        self.pmtmgmp = None
        self.base_mac = None
        self.base_route_table = None
        self.packet_size_map = {}
        self.clean_link_dict()


class Pmtmgmp_Route(object):
    def __init__(self, dummy_viz):
        self.viz = dummy_viz
        self.pmtmgmp = None
        self.show_msecp_mac = None
        self.show_mterp_mac = None
        self.show_data_send = None
        self.node_list = []
        self.mac_to_node = {}  # (str(Mac48Address), Pmtmgmp_Route)
        # self.last_scan_time = ns.core.Simulator.Now().GetSeconds()

    def route_path_clean(self):
        self.mac_to_node.clear()
        self.mac_to_node = {}
        for node in self.node_list:
            node.destory()
        del self.node_list[:]

        for node in self.viz.nodes.itervalues():
            ns3_node = ns.network.NodeList.GetNode(node.node_index)
            for devI in range(ns3_node.GetNDevices()):
                dev = ns3_node.GetDevice(devI)
                if isinstance(dev, ns.pmtmgmp.WmnPointDevice):
                    wmn_device = dev
                else:
                    continue
            pmtmgmp = wmn_device.GetRoutingProtocol()
            node_type = pmtmgmp.GetNodeType()
            node_color = 0xFF0000FF
            if (node_type == 2):
                node_color = 0x00FF00FF
            elif (node_type == 4):
                node_color = 0x0000FFFF
            elif (node_type == 8):
                node_color = 0xFFFF00FF
            if (pmtmgmp.GetPmtmgmpRouteTable().GetTableSize() == 0):
                node_color = 0xFFFFFFFF
            node.canvas_item.set_properties(fill_color_rgba=node_color)
            # if SHOW_LOG:
            #     print "Clean node list " + str(len(self.node_list))

    def route_path_link_full(self, msecp):
        # if SHOW_LOG:
        #     print "Pmtmgmp_Route::route_path_link_full() Node list:" + str(len(self.node_list))
        for node in self.node_list:
            node.crearte_route_path(self.mac_to_node, self.pmtmgmp.GetMacAddress(), msecp, self.viz.links_group)
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link_full()" + str(node.base_mac)

    def route_path_link_part(self, mterp):
        # if SHOW_LOG:
        #     print "Pmtmgmp_Route::route_path_link_part()"
        self.mac_to_node[str(self.pmtmgmp.GetMacAddress())].create_part_route_path_start(self.mac_to_node, mterp,
                                                                                         self.viz.links_group)

    def route_path_link_data(self):
        for node in self.node_list:
            node.crearte_data_path(self.mac_to_node, None, None, self.viz.links_group)
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link_data()"

    def route_path_link(self):
        # if SHOW_LOG:
        #     print "Pmtmgmp_Route::route_path_link().route_path_link_data"
        if (self.show_msecp_mac == None and self.show_mterp_mac == None and self.show_data_send == None):
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_clean"
            self.route_path_clean()
        elif self.show_mterp_mac != None:
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_link_part"
            self.route_path_link_part(self.show_mterp_mac)
        elif self.show_msecp_mac != None:
            # if (self.mac_to_node[str(self.pmtmgmp.GetMacAddress())].pmtmgmp.GetPmtmgmpRouteTable().GetPathByMACaddress(self.pmtmgmp.GetMacAddress(), self.show_msecp_mac) == 0):
            #     self. route_path_clean()
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_link_full"
            self.route_path_link_full(self.show_msecp_mac)
        elif self.show_data_send != None:
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_link_data"
            self.route_path_link_data()

    def scan_nodes(self, viz):
        self.viz = viz
        for node in self.viz.nodes.itervalues():
            ns3_node = ns.network.NodeList.GetNode(node.node_index)
            for devI in range(ns3_node.GetNDevices()):
                dev = ns3_node.GetDevice(devI)
                if isinstance(dev, ns.pmtmgmp.WmnPointDevice):
                    wmn_device = dev
                else:
                    continue
                new_node = Pmtmgmp_Node(node.node_index, node, wmn_device.GetRoutingProtocol(), viz)
                # if SHOW_LOG:
                #     print new_node
                self.node_list.append(new_node)
                self.mac_to_node[str(new_node.base_mac)] = new_node
        self.route_path_link()
        # if SHOW_LOG:
        #     print "scan_nodes " + str(len(self.node_list))

    def scan_and_clean_nodes(self, viz):
        self.route_path_clean()
        self.scan_nodes(viz)

    def simulation_periodic_update(self, viz):
        # if SHOW_LOG:
        #     print "simulation_periodic_update"
        # if (ns.core.Simulator.Now().GetSeconds() - self.last_scan_time > 1):
        #     self.scan_nodes(self.viz)
        #     self.last_scan_time = ns.core.Simulator.Now().GetSeconds()
        # if SHOW_LOG:
        #     print "Time Scan" + str(self.last_scan_time)
        self.route_path_link()

    def populate_node_menu(self, viz, node, menu):
        ns3_node = ns.network.NodeList.GetNode(node.node_index)
        wmn_device = None
        for devI in range(ns3_node.GetNDevices()):
            dev = ns3_node.GetDevice(devI)
            if isinstance(dev, ns.pmtmgmp.WmnPointDevice):
                wmn_device = dev
        if wmn_device == None:
            print "No PMTMGMP"
            return
        self.pmtmgmp = wmn_device.GetRoutingProtocol()

        # if SHOW_LOG:
        #     print self.pmtmgmp.GetNodeType()

        route = self
        route_table = self.pmtmgmp.GetPmtmgmpRouteTable()

        menu_item_main = gtk.MenuItem("Show Route")
        menu_item_main.show()
        menu_route = gtk.Menu()
        menu_route.show()
        menu_item_main.set_submenu(menu_route)
        add_menu = False

        def _clean_pmtmgmp_path(dummy_menu_item):
            route.show_msecp_mac = None
            route.show_mterp_mac = None
            route.show_data_send = None
            self.scan_nodes(viz)
            # if SHOW_LOG:
            #     print "Clean Path"

        menu_item = gtk.MenuItem("Clean")
        menu_item.show()
        menu_route.add(menu_item)
        menu_item.connect("activate", _clean_pmtmgmp_path)

        # As a STA
        route_table_size = route_table.GetTableSize()
        if route_table_size == 0 or (route_table_size == 1 and route_table.GetTableItem(
                0).GetMTERPaddress() == self.pmtmgmp.GetMacAddress()):
            if SHOW_LOG:
                print "There is no part path start at this node"
        else:
            add_menu = True
            # if SHOW_LOG:
            #     print "Find " + str(route_table_size) + " MTERP"

            menu_item = gtk.MenuItem("Show Pmtmgmp Route To Root")
            menu_item.show()
            menu_menu = gtk.Menu()
            menu_menu.show()
            menu_item.set_submenu(menu_menu)
            menu_route.add(menu_item)

            for i in range(0, route_table.GetTableSize()):
                route_tree = route_table.GetTableItem(i)

                def _show_pmtmgmp_part_path(dummy_menu_item, i):
                    route.show_msecp_mac = None
                    route.show_mterp_mac = route_table.GetTableItem(i).GetMTERPaddress()
                    route.show_data_send = None
                    self.scan_and_clean_nodes(viz)
                    if SHOW_LOG:
                        print "Show part path as MTERP is " + str(self.show_mterp_mac)

                menu_item_temp = gtk.MenuItem(str(route_tree.GetMTERPaddress()))
                menu_item_temp.show()
                menu_menu.add(menu_item_temp)
                menu_item_temp.connect("activate", _show_pmtmgmp_part_path, i)
        # if SHOW_LOG:
        #     print add_menu

        # As a MTERP
        route_tree = route_table.GetTreeByMACaddress(self.pmtmgmp.GetMacAddress())
        if route_tree == None:
            if SHOW_LOG:
                print "There is no full path start at this node while the MSECP is " + str(self.pmtmgmp.GetMacAddress())
        elif self.pmtmgmp.GetNodeType() & 0x06 == 0:
            if SHOW_LOG:
                print "This node is not MTERP"
        else:
            add_menu = True
            menu_item = gtk.MenuItem("Show Pmtmgmp Tree As Root")
            menu_item.show()
            menu_menu = gtk.Menu()
            menu_menu.show()
            menu_item.set_submenu(menu_menu)
            menu_route.add(menu_item)

            for i in range(0, route_tree.GetTreeSize()):

                def _show_pmtmgmp_full_path(dummy_menu_item, i):
                    route.show_mterp_mac = None
                    route.show_msecp_mac = route_tree.GetTreeItem(i).GetMSECPaddress()
                    route.show_data_send = None
                    self.scan_and_clean_nodes(viz)
                    if SHOW_LOG:
                        print "Show full path as MSECP is " + str(self.show_msecp_mac)

                route_path = route_tree.GetTreeItem(i)
                menu_item_temp = gtk.MenuItem(str(route_path.GetMSECPaddress()))
                menu_item_temp.show()
                menu_menu.add(menu_item_temp)
                menu_item_temp.connect("activate", _show_pmtmgmp_full_path, i)

        def _show_data_send_path(dummy_menu_item):
            route.show_mterp_mac = None
            route.show_msecp_mac = None
            route.show_data_send = route.pmtmgmp.GetMacAddress()
            self.scan_and_clean_nodes(viz)
            if SHOW_LOG:
                print "Show Data Send Path path"

        menu_item = gtk.MenuItem("Show Data Send Path path")
        menu_item.show()
        menu_route.add(menu_item)
        menu_item.connect("activate", _show_data_send_path)
        # if SHOW_LOG:
        #     print add_menu
        if add_menu:
            menu.add(menu_item_main)


pmtmgmp_route = None


def register(viz):
    global pmtmgmp_route
    if pmtmgmp_route == None:
        pmtmgmp_route = Pmtmgmp_Route(viz)
    viz.connect("populate-node-menu", pmtmgmp_route.populate_node_menu)
    viz.connect("simulation-periodic-update", pmtmgmp_route.simulation_periodic_update)
    viz.connect("update-view", pmtmgmp_route.simulation_periodic_update)
    viz.connect("topology-scanned", pmtmgmp_route.scan_and_clean_nodes)

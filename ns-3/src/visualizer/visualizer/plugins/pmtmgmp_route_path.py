import gtk
import math
import goocanvas
import ns.core
import ns.network
import ns.visualizer
import ns.pmtmgmp

from visualizer.base import Link, transform_distance_canvas_to_simulation
SHOW_LOG = True
COLOR = [0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF, 0xFF00FFFF, 0xFFFF00FF, 0x8A2BE2FF, 0xB8860BFF, 0x006400FF, 0xFF1493FF]
POINT_MODIFY = 10
class Pmtmgmp_Path_Link(Link):
    def __init__(self, next_link, base_node, link_node, base_route_table, mterp, msecp, parent_canvas_item, color):
        self.base_node = base_node
        self.link_node = link_node
        self.base_route_table = base_route_table
        self.mterp = mterp
        self.msecp = msecp
        self.canvas_item = goocanvas.Group(parent=parent_canvas_item)
        # self.line = goocanvas.Polyline(parent=self.canvas_item, line_width=1.0, stroke_color_rgba=0xC00000FF, start_arrow=True, arrow_length=10,arrow_width=8)
        self.line = goocanvas.Polyline(parent=self.canvas_item, line_width=2.0, stroke_color_rgba=color, start_arrow=True, close_path=False, end_arrow=False, arrow_length=15, arrow_width=15)
        self.line.raise_(None)
        self.canvas_item.set_data("pyviz-object", self)
        self.canvas_item.lower(None)
        self.way_index = 0
        self.next_link = next_link

    def set_way_index(self, index):
        self.way_index = index

    def set_next_link(self, link):
        next_link = link

    def set_color(self, color):
        self.line.set_properties(stroke_color_rgba=color)

    def get_move_node(self):
        pos1_x, pos1_y = self.link_node.viz_node.get_position()
        pos2_x, pos2_y = self.base_node.viz_node.get_position()
        if (self.way_index == 0):
            return goocanvas.Points([(pos1_x, pos1_y), (pos2_x, pos2_y)])
        else:
            angle = math.atan2(pos2_y - pos1_y, pos2_x - pos1_x) + math.pi / 2
            x = POINT_MODIFY * math.cos(angle)
            y = POINT_MODIFY * math.sin(angle)
            if self.next_link != None:
                self.next_index = self.next_link.way_index
            return goocanvas.Points([(pos1_x + x * self.next_index, pos1_y + y * self.next_index), (pos2_x + x * self.way_index, pos2_y + y * self.way_index)])

    def update_points(self):
        if self.base_route_table.GetPathByMACaddress(self.mterp, self.msecp) == None:
            if SHOW_LOG:
                print "Delete(Path) " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
            self.destroy()
        elif (self.base_node == None) or (self.link_node == None):
            if SHOW_LOG:
                print "Delete(MAC) " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
            self.destroy()
        else:
            # if SHOW_LOG:
            #     print "Move " + str(self.base_node.pmtmgmp.GetMacAddress()) + " to " + str(self.link_node.pmtmgmp.GetMacAddress())
            self.line.set_property("points", self.get_move_node())

    def destroy(self):
        self.line.remove()
        self.canvas_item.remove()
        self.ns_node = None
        self.pmtmgmp_node = None
        del(self)

class Pmtmgmp_Node(object):
    def __init__(self, node_index, viz_node, pmtmgmp):
        self.node_index = node_index
        self.viz_node = viz_node
        self.base_mac = pmtmgmp.GetMacAddress()
        self.base_route_table = pmtmgmp.GetPmtmgmpRPRouteTable()
        self.pmtmgmp = pmtmgmp
        self.link_list = {}

    def set_color(self, color):
        self.viz_node.canvas_item.set_properties(fill_color_rgba=color)

    def clean_link_list(self):
        for (key,link) in self.link_list.items():
            link.destroy()
            del(self.link_list[key])
        self.link_list.clear()
        self.link_list = {}

    def update_link_list(self):
        i = 0
        for (key,link) in self.link_list.items():
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
            color = 0x0000FFFF
        if (self.pmtmgmp.GetPmtmgmpRPRouteTable().GetTableSize() == 0):
            color = 0xFFFFFFFF
        self.set_color(color)

        route_table = self.pmtmgmp.GetPmtmgmpRPRouteTable()
        route_path = route_table.GetPathByMACaddress(mterp,msecp)
        if route_path == None:
             return
        if self.link_list.has_key(str(mterp) + "," + str(msecp)):
            self.link_list[str(mterp) + "," + str(msecp)].update_points()
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::crearte_route_path() exist " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        else:
            if mac_node_list[str(route_path.GetFromNode())] is self:
                return
            route_link = Pmtmgmp_Path_Link(None, self, mac_node_list[str(route_path.GetFromNode())], route_table, mterp, msecp, parent_canvas_item, COLOR[1])
            self.link_list[str(mterp) + "," + str(msecp)] = route_link
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::crearte_route_path() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        self.update_link_list()

    def create_part_route_path_recursion(self, mac_node_list, mterp, msecp, parent_canvas_item, color):
        node_type = self.pmtmgmp.GetNodeType()
        node_color = 0xFF0000FF
        if (node_type == 2):
            node_color = 0x00FF00FF
        elif (node_type == 4):
            node_color = 0x0000FFFF
        elif (node_type == 8):
            node_color = 0x0000FFFF
        if (self.pmtmgmp.GetPmtmgmpRPRouteTable().GetTableSize() == 0):
            node_color = 0xFFFFFFFF
        self.set_color(node_color)
        route_table = self.pmtmgmp.GetPmtmgmpRPRouteTable()
        route_path = route_table.GetPathByMACaddress(mterp,msecp)
        if route_path == None:
             return None
        next_hop = mac_node_list[str(route_path.GetFromNode())]
        if next_hop is self:
            return None
        next_link = next_hop.create_part_route_path_recursion(mac_node_list, mterp, msecp, parent_canvas_item, color)
        key = str(mterp) + "," + str(msecp)
        if self.link_list.has_key(key):
            self.link_list[key].set_color(color)
            self.link_list[key].set_next_link(next_link)
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::create_part_route_path_recursion() exist " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        else:
            self.link_list[key] = Pmtmgmp_Path_Link(next_link, self, mac_node_list[str(route_path.GetFromNode())], route_table, mterp, msecp, parent_canvas_item, color)
            # if SHOW_LOG:
            #     print "Pmtmgmp_Node::create_part_route_path_recursion() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
        self.update_link_list()
        return self.link_list[key]

    def create_part_route_path_start(self, mac_node_list, mterp, parent_canvas_item):
        node_type = self.pmtmgmp.GetNodeType()
        node_color = 0xFF0000FF
        if (node_type == 2):
            node_color = 0x00FF00FF
        elif (node_type == 4):
            node_color = 0x0000FFFF
        elif (node_type == 8):
            node_color = 0x0000FFFF
        if (self.pmtmgmp.GetPmtmgmpRPRouteTable().GetTableSize() == 0):
            node_color = 0xFFFFFFFF
        self.set_color(node_color)
        route_table = self.pmtmgmp.GetPmtmgmpRPRouteTable()
        route_tree = route_table.GetTreeByMACaddress(mterp)
        if route_tree == None:
            return
        for i in range(0, route_tree.GetTreeSize()):
            route_path = route_tree.GetTreeItem(i)
            msecp = route_path.GetMSECPaddress()
            next_hop = mac_node_list[str(route_path.GetFromNode())]
            if next_hop is self:
                return None
            next_link = next_hop.create_part_route_path_recursion(mac_node_list, mterp, msecp, parent_canvas_item, COLOR[i%10])
            key = str(mterp) + "," + str(msecp)
            if self.link_list.has_key(key):
                self.link_list[key].set_color(COLOR[i%10])
                self.link_list[key].set_next_link(next_link)
                # if SHOW_LOG:
                #     print "Pmtmgmp_Node::create_part_route_path_recursion() exist " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
            else:
                self.link_list[key] = Pmtmgmp_Path_Link(next_link, self, mac_node_list[str(route_path.GetFromNode())], route_table, mterp, msecp, parent_canvas_item, COLOR[i%10])
                # if SHOW_LOG:
                #     print "Pmtmgmp_Node::create_part_route_path_recursion() " + str(self.pmtmgmp.GetMacAddress()) + " link to " + str(route_path.GetFromNode())
            self.update_link_list()

    def destory(self):
        self.node_index  = None
        self.viz_node  = None
        self.pmtmgmp  = None
        self.base_mac = None
        self.base_route_table = None
        self.clean_link_list()

class Pmtmgmp_Route(object):
    def __init__(self, dummy_viz):
        self.viz = dummy_viz
        self.pmtmgmp = None
        self.show_msecp_mac = None
        self.show_mterp_mac = None
        self.node_list = []
        self.mac_to_node = {} #(str(Mac48Address), Pmtmgmp_Route)
        self.last_scan_time = ns.core.Simulator.Now().GetSeconds()

    def route_path_clean(self):
        self.mac_to_node.clear()
        self.mac_to_node = {}
        for node in self.node_list:
            node.destory()
        del self.node_list[:]
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
        self.mac_to_node[str(self.pmtmgmp.GetMacAddress())].create_part_route_path_start(self.mac_to_node, mterp, self.viz.links_group)

    def route_path_link(self):
        if (self.show_msecp_mac == None and self.show_mterp_mac == None):
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_clean"
            self. route_path_clean()
        elif self.show_msecp_mac == None:
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_link_part"
            self.route_path_link_part(self.show_mterp_mac)
        elif self.show_mterp_mac == None:
            # if (self.mac_to_node[str(self.pmtmgmp.GetMacAddress())].pmtmgmp.GetPmtmgmpRPRouteTable().GetPathByMACaddress(self.pmtmgmp.GetMacAddress(), self.show_msecp_mac) == 0):
            #     self. route_path_clean()
            # if SHOW_LOG:
            #     print "Pmtmgmp_Route::route_path_link().route_path_link_full"
            self.route_path_link_full(self.show_msecp_mac)

    def scan_nodes(self, viz):
        self. route_path_clean()
        for node in viz.nodes.itervalues():
            ns3_node = ns.network.NodeList.GetNode(node.node_index)
            for devI in range(ns3_node.GetNDevices()):
                dev = ns3_node.GetDevice(devI)
                if isinstance(dev, ns.pmtmgmp.WmnPointDevice):
                    WmnDevice = dev
                else:
                    continue
                new_node = Pmtmgmp_Node(node.node_index, node, WmnDevice.GetRoutingProtocol())
                # if SHOW_LOG:
                #     print new_node
                self.node_list.append(new_node)
                self.mac_to_node[str(new_node.base_mac)] = new_node
        self.route_path_link()
        # if SHOW_LOG:
        #     print "scan_nodes " + str(len(self.node_list))

    def simulation_periodic_update(self, viz):
        # if SHOW_LOG:
        #     print "simulation_periodic_update"
        if (ns.core.Simulator.Now().GetSeconds() - self.last_scan_time > 60):
            self.scan_nodes(self.viz)
            self.last_scan_time = ns.core.Simulator.Now().GetSeconds()
            if SHOW_LOG:
                print "Time Scan" + str(self.last_scan_time)
        self.route_path_link()

    def populate_node_menu(self, viz, node, menu):
        ns3_node = ns.network.NodeList.GetNode(node.node_index)
        WmnDevice = None
        for devI in range(ns3_node.GetNDevices()):
            dev = ns3_node.GetDevice(devI)
            if isinstance(dev, ns.pmtmgmp.WmnPointDevice):
                WmnDevice = dev
        if WmnDevice == None:
            print "No PMTMGMP"
            return
        self.pmtmgmp = WmnDevice.GetRoutingProtocol()

        # if SHOW_LOG:
        #     print self.pmtmgmp.GetNodeType()

        route = self
        route_table = self.pmtmgmp.GetPmtmgmpRPRouteTable()

        menu_item_main = gtk.MenuItem("Show Route")
        menu_item_main.show()
        menu_route = gtk.Menu()
        menu_route.show()
        menu_item_main.set_submenu(menu_route)
        add_menu = False

        def _clean_pmtmgmp_path(dummy_menu_item):
            route.show_msecp_mac = None
            route.show_mterp_mac = None
            self.scan_nodes(viz)
            # if SHOW_LOG:
            #     print "Clean Path"

        menu_item = gtk.MenuItem("Clean")
        menu_item.show()
        menu_route.add(menu_item)
        menu_item.connect("activate", _clean_pmtmgmp_path)

        #As a STA
        route_table_size = route_table.GetTableSize()
        if route_table_size == 0 or (route_table_size == 1 and route_table.GetTableItem(0).GetMTERPaddress() == self.pmtmgmp.GetMacAddress()):
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
                    self.scan_nodes(viz)
                    if SHOW_LOG:
                        print "Show part path as MTERP is " + str(self.show_mterp_mac)

                menu_item_temp = gtk.MenuItem (str(route_tree.GetMTERPaddress()))
                menu_item_temp.show()
                menu_menu.add(menu_item_temp)
                menu_item_temp.connect("activate", _show_pmtmgmp_part_path, i)
        # if SHOW_LOG:
        #     print add_menu

        #As a MTERP
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
                    self.scan_nodes(viz)
                    if SHOW_LOG:
                        print "Show full path as MSECP is " + str(self.show_msecp_mac)

                route_path = route_tree.GetTreeItem(i)
                menu_item_temp = gtk.MenuItem (str(route_path.GetMSECPaddress()))
                menu_item_temp.show()
                menu_menu.add(menu_item_temp)
                menu_item_temp.connect("activate", _show_pmtmgmp_full_path, i)
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
    viz.connect("topology-scanned", pmtmgmp_route.scan_nodes)
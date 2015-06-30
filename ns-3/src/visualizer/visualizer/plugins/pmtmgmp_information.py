import gtk

import ns.core
import ns.network
import ns.visualizer
import ns.pmtmgmp

from visualizer.base import InformationWindow
from zmq.backend.cython.constants import NULL


class ShowPmtmgmpInforamtion(InformationWindow):
    def __init__(self, visualizer, node_index ,device):
        InformationWindow.__init__(self)
        self.win = gtk.Dialog(parent=visualizer.window,
                              flags=gtk.DIALOG_DESTROY_WITH_PARENT|gtk.DIALOG_NO_SEPARATOR,
                              buttons=(gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))
        self.win.set_default_size(gtk.gdk.screen_width()/2, gtk.gdk.screen_height()/2)
        self.win.connect("response", self._response_cb)
        self.win.set_title("PMTMGMP information for node %i" % node_index) 
        self.visualizer = visualizer
        self.node_index = node_index
        self.table_model = gtk.ListStore(str, str, int, int, int, float, bool)

        self.pmtmgmp = device.GetRoutingProtocol()
        type_index = self.pmtmgmp.GetNodeType()
        type_str = ""

		#Information label of Node
        if type_index == 0:
            type_str = "Mesh_STA"
        if type_index == 1:
            type_str = "Mesh_Access_Point"
        if type_index == 2:
            type_str = "Mesh_Portal"
        if type_index == 3:
            type_str = "Mesh_Secondary_Point"
        label = gtk.Label("Information For Node[" + bytes(self.node_index) + "]:" + type_str)
        label.show()

        vbox = gtk.VBox()
        hbox = gtk.HBox()
        hbox.pack_start(label, False, False, 3)
        hbox.show()
        vbox.pack_start(hbox, False, True, 0)
        h_eparator = gtk.HSeparator()
        h_eparator.show()
        vbox.pack_start(h_eparator, False, True, 3)

        #NoteBook
        self.notebook = gtk.Notebook()
        self.notebook.show()

        self.visualizer.add_information_window(self)
        self.win.show()
        
    def _response_cb(self, win, response):
        self.win.destroy()
        self.visualizer.remove_information_window(self)
    
    def update(self):
        route_table = self.pmtmgmp.GetPmtmgmpRPRouteTable()

def populate_node_menu(viz, node, menu):    
    ns3_node = ns.network.NodeList.GetNode(node.node_index)
    WmnDevice = None
    for devI in range(ns3_node.GetNDevices()):
        dev = ns3_node.GetDevice(devI)
        name = ns.core.Names.FindName(dev)
        if dev.GetInstanceTypeId().GetName() == "ns3::WmnPointDevice":
            WmnDevice = dev
    if WmnDevice is None:
        print "No PMTMGMP"
        return

    menu_item = gtk.MenuItem("Show Pmtmgmp Information")
    menu_item.show()
    
    def _show_pmtmgmp_information(dummy_menu_item):
        ShowPmtmgmpInforamtion(viz, node.node_index, WmnDevice)


    menu_item.connect("activate", _show_pmtmgmp_information)
    menu.add(menu_item)

def register(viz):
    viz.connect("populate-node-menu", populate_node_menu)
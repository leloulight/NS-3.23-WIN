import gtk

import ns.core
import ns.network
import ns.visualizer
import ns.pmtmgmp

from visualizer.base import InformationWindow
from zmq.backend.cython.constants import NULL


class ShowPmtmgmpInforamtion(InformationWindow):
    (
        COLUMN_MESH_POINT_TYPE,
        COLUMN_RT_RETRANSMITTER,
        COLUMN_RT_INTERFACE,
        COLUMN_RT_METRIC,
        COLUMN_RT_SEQUENCENUMBER,
        COLUMN_RT_LIFETIME,
        COLUMN_RT_VALID
        ) = range(7)
        
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
        type_index = pmtmgmp.GetNodeType()
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
        lable = gtk.Label("Information For Node[" + bytes(self.node_index) + "]:" + type_str)
		label.show()
		
		vbox = gtk.VBox()
		hbox = gtk.HBox()
		hbox.pack_start(label, False, False, 3)
		hbox.show()
		vbox.pack_start(hbox, False, True, 0)h_eparator = gtk.HSeparator()
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
        node = ns.network.NodeList.GetNode(self.node_index)
        WmnDevice = None
        for devI in range(node.GetNDevices()):
            dev = node.GetDevice(devI)
            name = ns.core.Names.FindName(dev)
            if dev.GetInstanceTypeId().GetName() == "ns3::WmnPointDevice":
                WmnDevice = dev
        if WmnDevice is None:
            return
        
        pmtmgmp = WmnDevice.GetRoutingProtocol()
        self.table_model.clear()
        
        # Mesh Point Type.
        tree_iter = self.table_model.append()
        index = pmtmgmp.GetNodeType()
        typestr = ""
        if index == 0:
            typestr = "Mesh_STA"
        if index == 1:
            typestr = "Mesh_Access_Point"
        if index == 2:
            typestr = "Mesh_Portal"
        if index == 3:
            typestr = "Mesh_Secondary_Point"
            
        #self.table_model.set(tree_iter,
                                 #self.COLUMN_MESH_POINT_TYPE, typestr,
                                 #self.COLUMN_RT_RETRANSMITTER, rtable.retransmitter,
                                 #self.COLUMN_RT_INTERFACE, rtable.ifIndex,
                                 #self.COLUMN_RT_METRIC, rtable.metric,
                                 #self.COLUMN_RT_SEQUENCENUMBER, rtable.seqnum,
                                 #self.COLUMN_RT_LIFETIME, rtable.lifetime.GetSeconds(),
                                 #self.COLUMN_RT_VALID, rtable.IsValid())

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
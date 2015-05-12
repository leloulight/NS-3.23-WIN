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
        
    def __init__(self, visualizer, node_index):
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

        treeview = gtk.TreeView(self.table_model)
        treeview.show()
        sw = gtk.ScrolledWindow()
        sw.set_properties(hscrollbar_policy=gtk.POLICY_AUTOMATIC,
                          vscrollbar_policy=gtk.POLICY_AUTOMATIC)
        sw.show()
        sw.add(treeview)
        self.win.vbox.add(sw)
        
        # Mesh Point Type.
        column = gtk.TreeViewColumn('Mesh Point Type', gtk.CellRendererText(),
                                    text=self.COLUMN_MESH_POINT_TYPE)
        treeview.append_column(column)

        # Route Table Retransmitter.
        column = gtk.TreeViewColumn('Route Table Retransmitter', gtk.CellRendererText(),
                                    text=self.COLUMN_RT_RETRANSMITTER)
        treeview.append_column(column)

        # Route Table Interface.
        column = gtk.TreeViewColumn('Route Table Interface', gtk.CellRendererText(),
                                    text=self.COLUMN_RT_INTERFACE)
        treeview.append_column(column)

        # Route Table METRIC.
        column = gtk.TreeViewColumn('Route Table METRIC', gtk.CellRendererText(),
                                    text=self.COLUMN_RT_METRIC)
        treeview.append_column(column)

        # Route Table Sequence Number.
        column = gtk.TreeViewColumn('Route Table Sequence Number', gtk.CellRendererText(),
                                    text=self.COLUMN_RT_SEQUENCENUMBER)
        treeview.append_column(column)
        
        # Route Table Life Time.
        column = gtk.TreeViewColumn('Route Table Life Time', gtk.CellRendererText(),
                                    text=self.COLUMN_RT_LIFETIME)
        treeview.append_column(column)
        
        # Route Table Valid.
        column = gtk.TreeViewColumn('Route Table Valid', gtk.CellRendererText(),
                                    text=self.COLUMN_RT_VALID)
        treeview.append_column(column)

        self.visualizer.add_information_window(self)
        self.win.show()
        
    def _response_cb(self, win, response):
        self.win.destroy()
        self.visualizer.remove_information_window(self)
    
    def update(self):
        node = ns.network.NodeList.GetNode(self.node_index)
        pmtmgmp = None
        for devI in range(node.GetNDevices()):
            dev = node.GetDevice(devI)
            name = ns.core.Names.FindName(dev)
            if dev.GetInstanceTypeId().GetName() == "ns3::PmtmgmpPointDevice":
                pmtmgmp = dev
        if pmtmgmp is None:
            return
        rtable = pmtmgmp.GetRoutingProtocol().GetPmtmgmpRtable().LookupProactive();
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
            
        self.table_model.set(tree_iter,
                                 self.COLUMN_MESH_POINT_TYPE, typestr,
                                 self.COLUMN_RT_RETRANSMITTER, rtable.retransmitter,
                                 self.COLUMN_RT_INTERFACE, rtable.ifIndex,
                                 self.COLUMN_RT_METRIC, rtable.metric,
                                 self.COLUMN_RT_SEQUENCENUMBER, rtable.seqnum,
                                 self.COLUMN_RT_LIFETIME, rtable.lifetime.GetSeconds(),
                                 self.COLUMN_RT_VALID, rtable.IsValid())

def populate_node_menu(viz, node, menu):    
    ns3_node = ns.network.NodeList.GetNode(node.node_index)
    pmtmgmp = None
    for devI in range(ns3_node.GetNDevices()):
        dev = ns3_node.GetDevice(devI)
        name = ns.core.Names.FindName(dev)
        if dev.GetInstanceTypeId().GetName() == "ns3::PmtmgmpPointDevice":
            pmtmgmp = dev
    if pmtmgmp is None:
        print "No PMTMGMP"
        return

    menu_item = gtk.MenuItem("Show Pmtmgmp Information")
    menu_item.show()
    
    def _show_pmtmgmp_information(dummy_menu_item):
        ShowPmtmgmpInforamtion(viz, node.node_index)


    menu_item.connect("activate", _show_pmtmgmp_information)
    menu.add(menu_item)

def register(viz):
    viz.connect("populate-node-menu", populate_node_menu)
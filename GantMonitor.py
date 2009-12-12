#!/usr/bin/env python

import sys
import os
import time

import gtk
import gtk.glade
import gobject 
import vte

import pygtk
pygtk.require("2.0")

class DownloadDialog:
	"""This class is used to show DownloadDialog"""
	
	def __init__(self):
		self.gladefile = "GantMonitor.glade"
		self.wTree = gtk.glade.XML(self.gladefile)
	
	def run(self):
		"""Configures and runs the download dialog"""
		self.wTree = gtk.glade.XML(self.gladefile, "DownloadDialog") 
		events = { "on_expand_details" : self.on_expand_details, "on_restart_process":self.on_restart_process}
		self.wTree.signal_autoconnect(events)
		
		self.dlg = self.wTree.get_widget("DownloadDialog")
		
		self.close_button = self.wTree.get_widget("btn_close")
		self.close_button.set_use_stock(True)
		
		self.details_section = self.wTree.get_widget("details_section")
		self.status_label = self.wTree.get_widget("lbl_status")
		
		self.progress_bar = self.wTree.get_widget("progressbar")
		self.progress_bar.pulse()
		self.timeout_handler_id = gobject.timeout_add(100, self.update_progress_bar)
		self.start = time.time()
		
		terminal=  vte.Terminal()
		terminal.connect("show", self.on_show_terminal)
		terminal.connect('child-exited', self.on_child_exited)
		self.details_section.add(terminal)
				
		self.dlg.show_all()
		self.result =  self.dlg.run()
		self.dlg.destroy()
		
	def update_progress_bar(self):
		self.progress_bar.pulse()
		self.status_label.set_text("Gant running... (pid: " + str(self.child_pid) + ") " + time.asctime() )
		return True
	
	def on_show_terminal(self, terminal):
		self.start_gant(terminal)
	
	def start_gant(self, terminal):
		self.child_pid = terminal.fork_command("./gant" , argv = [' -p'] )
		
	def on_child_exited(self, child):
		"""Updates label after download complete"""
		child.destroy()
		self.status_label.set_text("Gant download complete!")
		self.close_button.set_label(gtk.STOCK_CLOSE)
		self.close_button.set_use_stock(True)
		print "gant exited"
	
	# doesn't work, dunno why...
	def on_expand_details(self, expander):
		if not expander.get_expanded():
			self.dlg.resize(415,130)
	
	#doesn't work, i think gtk dialog buttons are wired to only close the dialog... refactor?
	def on_restart_process(self, button):
		os.kill(self.child_id, signal.SIGSTOP)
		self.start_gant(self.terminal)
		
class SettingsDialog:
	"""This class is used to show SettingsDialog, which has no purpose (yet)"""
	
	def __init__(self):
		self.gladefile = "GantMonitor.glade"
		self.wTree = gtk.glade.XML(self.gladefile) 
		
	def run(self):
		self.wTree = gtk.glade.XML(self.gladefile, "SettingsDialog") 
		self.dlg = self.wTree.get_widget("SettingsDialog")
		self.result = self.dlg.run()
		self.dlg.destroy()

class GantMonitorStatusIcon(gtk.StatusIcon):
	"""This class is used to show the tray icon and the menu"""
	
	def __init__(self):
		gtk.StatusIcon.__init__(self)
		icon_filename = 'resources/gant.png'
		menu = '''
			<ui>
			 <menubar name="Menubar">
			  <menu action="Menu">
			   <menuitem action="Download"/>
			   <menuitem action="Preferences"/>
			   <separator/>
			   <menuitem action="About"/>
			  <separator/>
			   <menuitem action="Exit"/> 
			  </menu>
			 </menubar>
			</ui>'''
		
		actions = [
			('Menu',  None, 'Menu'),
			('Download',icon_filename, '_Download...', None, 'Download data using Gant', self.on_download),
			('Preferences', gtk.STOCK_PREFERENCES, '_Preferences...', None, 'Change Gant Monitor preferences', self.on_preferences),
			('About', gtk.STOCK_ABOUT, '_About...', None, 'About Gant Monitor', self.on_about),
			('Exit', gtk.STOCK_QUIT, '_Exit...', None, 'Exit Gant Monitor', self.on_exit)
			]
		
		ag = gtk.ActionGroup('Actions')
		ag.add_actions(actions)
		
		self.manager = gtk.UIManager()
		self.manager.insert_action_group(ag, 0)
		self.manager.add_ui_from_string(menu)
		self.menu = self.manager.get_widget('/Menubar/Menu/About').props.parent
		
		search = self.manager.get_widget('/Menubar/Menu/Download')
		search.get_children()[0].set_markup('<b>_Download...</b>')
		search.get_children()[0].set_use_underline(True)
		search.get_children()[0].set_use_markup(True)
		
		self.set_from_file(icon_filename)
		self.set_tooltip('Gant Monitor')
		self.set_visible(True)
		self.connect('activate', self.on_download)
		self.connect('popup-menu', self.on_popup_menu)

	def on_download(self, data):
		downloadDialog=DownloadDialog()
		downloadDialog.run()
		
	def on_exit(self, data):
		gtk.main_quit()

	def on_popup_menu(self, status, button, time):
		self.menu.popup(None, None, None, button, time)

	# configure default cmd line params and serialize to xml?
	def on_preferences(self, data):
		print 'Gant preferences - todo'
		settings = SettingsDialog()
		settings.run()
		print settings.result

	def on_about(self, data):
		dialog = gtk.AboutDialog()
		dialog.set_name('Gant Monitor')
		dialog.set_version('0.1.0')
		dialog.set_comments('A tray icon to start Gant')
		dialog.set_website('http://cgit.get-open.com/cgit.cgi/gant/')
		dialog.run()
		dialog.destroy()

if __name__ == '__main__':
	GantMonitorStatusIcon()
	gtk.main()

# -*- Makefile -*- for Darwinia

#
# In the parent directory of code/ contrib/ targets/
#
# make -f targets/linux/Makefile
# 

CXX=clang
CC=clang
CXXFLAGS=-g -Os

CPPFLAGS=\
-DTARGET_OS_LINUX \
-DHAVE_SOCKET \
-DHAVE_SELECT \
-DHAVE_GETHOSTBYNAME \
-DHAVE_INET_NTOA \
-Icode \
-Icontrib/eclipse \
-Icontrib/unrar \
-Icontrib/netclass \
-I/usr/include/SDL \
-Icontrib/binreloc 

#-D_DEBUG

# CONTRIBDEPS=\
# contrib/unrar/libunrar.a\
# contrib/netclass/libnetclass.a

# $(CONTRIBDEPS):
# 	@echo "Building contrib dependency: " $@
# 	@$(MAKE) -C $(dir $@) $(notdir $@)

LIBS=\
-lSDL\
-L/usr/X11R6/lib -lGL -lGLU \
-lvorbisfile -lvorbis -logg \

LINK=$(CXX)

SOURCES=\
code/3d_sierpinski_gasket.cpp \
code/app.cpp \
code/camera.cpp \
code/clouds.cpp \
code/control_bindings.cpp \
code/effect_processor.cpp \
code/entity_grid.cpp \
code/explosion.cpp \
code/gesture.cpp \
code/gesture_demo.cpp \
code/gesture_demo_sdl.cpp \
code/global_internet.cpp \
code/global_world.cpp \
code/gamecursor.cpp \
code/helpsystem.cpp \
code/interface/buildings_window.cpp \
code/interface/camera_anim_window.cpp \
code/interface/camera_mount_window.cpp \
code/interface/cheat_window.cpp \
code/interface/darwinia_window.cpp \
code/interface/debugmenu.cpp \
code/interface/drop_down_menu.cpp \
code/interface/editor_window.cpp \
code/interface/filedialog.cpp \
code/interface/gesture_window.cpp \
code/interface/globalworldeditor_window.cpp \
code/interface/grabber_window.cpp \
code/interface/input_field.cpp \
code/interface/inputlogger_window.cpp \
code/interface/instant_unit_window.cpp \
code/interface/landscape_window.cpp \
code/interface/lights_window.cpp \
code/interface/mainmenus.cpp \
code/interface/message_dialog.cpp \
code/interface/mods_window.cpp \
code/interface/network_window.cpp \
code/interface/pokey_window.cpp \
code/interface/prefs_other_window.cpp \
code/interface/prefs_sound_window.cpp \
code/interface/prefs_graphics_window.cpp \
code/interface/prefs_screen_window.cpp \
code/interface/prefs_keybindings_window.cpp \
code/interface/profilewindow.cpp \
code/interface/reallyquit_window.cpp \
code/interface/save_on_quit_window.cpp \
code/interface/scrollbar.cpp \
code/interface/sound_profile_window.cpp \
code/interface/soundeditor_window.cpp \
code/interface/soundparameter_window.cpp \
code/interface/soundstats_window.cpp \
code/interface/tree_window.cpp \
code/interface/userprofile_window.cpp \
code/keygen.cpp \
code/landscape.cpp \
code/landscape_renderer.cpp \
code/level_file.cpp \
code/lib/3d_sprite.cpp \
code/lib/avi_generator.cpp \
code/lib/binary_stream_readers.cpp \
code/lib/bitmap.cpp \
code/lib/debug_render.cpp \
code/lib/debug_utils_linux.cpp \
code/lib/file_writer.cpp \
code/lib/filesys_utils.cpp \
code/lib/hi_res_time_sdl.cpp \
code/lib/input_sdl.cpp \
code/lib/invert_matrix.cpp \
code/lib/language_table.cpp \
code/lib/math_utils.cpp \
code/lib/matrix33.cpp \
code/lib/matrix34.cpp \
code/lib/mouse_cursor.cpp \
code/lib/ogl_extensions_sdl.cpp \
code/lib/persisting_debug_render.cpp \
code/lib/plane.cpp \
code/lib/poster_maker.cpp \
code/lib/preferences.cpp \
code/lib/profiler.cpp \
code/lib/random.cpp \
code/lib/render_utils.cpp \
code/lib/resource.cpp \
code/lib/rgb_colour.cpp \
code/lib/shape.cpp \
code/lib/sphere_renderer.cpp \
code/lib/string_utils.cpp \
code/lib/system_info_linux.cpp \
code/lib/text_renderer.cpp \
code/lib/text_stream_readers.cpp \
code/lib/texture_uv.cpp \
code/lib/tri_tri.cpp \
code/lib/universal_include.cpp \
code/lib/user_info.cpp \
code/lib/vector2.cpp \
code/lib/vector3.cpp \
code/lib/discard_mouse_events.cpp \
code/lib/window_manager_sdl.cpp \
code/loaders/fodder_loader.cpp \
code/loaders/gameoflife_loader.cpp \
code/loaders/loader.cpp \
code/loaders/matrix_loader.cpp \
code/loaders/raytrace_loader.cpp \
code/loaders/soul_loader.cpp \
code/loaders/speccy_loader.cpp \
code/loaders/credits_loader.cpp \
code/location.cpp \
code/location_editor.cpp \
code/location_input.cpp \
code/main.cpp \
code/network/clienttoserver.cpp \
code/network/generic.cpp \
code/network/networkupdate.cpp \
code/network/server.cpp \
code/network/servertoclient.cpp \
code/network/servertoclientletter.cpp \
code/obstruction_grid.cpp \
code/particle_system.cpp \
code/renderer.cpp \
code/routing_system.cpp \
code/script.cpp \
code/sepulveda.cpp \
code/sound/pokey.cpp \
code/sound/sample_cache.cpp \
code/sound/sound_filter.cpp \
code/sound/sound_instance.cpp \
code/sound/sound_library_2d_sdl.cpp \
code/sound/sound_library_2d_lock_sdl.cpp \
code/sound/sound_library_3d.cpp \
code/sound/sound_library_3d_software.cpp \
code/sound/sound_parameter.cpp \
code/sound/sound_stream_decoder.cpp \
code/sound/soundsystem.cpp \
code/startsequence.cpp \
code/taskmanager.cpp \
code/team.cpp \
code/testharness.cpp \
code/tutorial.cpp \
code/unit.cpp \
code/user_input.cpp \
code/water.cpp \
code/worldobject/ai.cpp \
code/worldobject/airstrike.cpp \
code/worldobject/anthill.cpp \
code/worldobject/armour.cpp \
code/worldobject/armyant.cpp \
code/worldobject/blueprintstore.cpp \
code/worldobject/bridge.cpp \
code/worldobject/building.cpp \
code/worldobject/cave.cpp \
code/worldobject/centipede.cpp \
code/worldobject/constructionyard.cpp \
code/worldobject/controltower.cpp \
code/worldobject/darwinian.cpp \
code/worldobject/egg.cpp \
code/worldobject/engineer.cpp \
code/worldobject/entity.cpp \
code/worldobject/entity_leg.cpp \
code/worldobject/factory.cpp \
code/worldobject/flag.cpp \
code/worldobject/goddish.cpp \
code/worldobject/generator.cpp \
code/worldobject/gunturret.cpp \
code/worldobject/incubator.cpp \
code/worldobject/insertion_squad.cpp \
code/worldobject/lander.cpp \
code/worldobject/laserfence.cpp \
code/worldobject/lasertrooper.cpp \
code/worldobject/library.cpp \
code/worldobject/mine.cpp \
code/worldobject/officer.cpp \
code/worldobject/powerstation.cpp \
code/worldobject/radardish.cpp \
code/worldobject/researchitem.cpp \
code/worldobject/safearea.cpp \
code/worldobject/scripttrigger.cpp \
code/worldobject/snow.cpp \
code/worldobject/souldestroyer.cpp \
code/worldobject/spam.cpp \
code/worldobject/spawnpoint.cpp \
code/worldobject/spider.cpp \
code/worldobject/spirit.cpp \
code/worldobject/spiritreceiver.cpp \
code/worldobject/spiritstore.cpp \
code/worldobject/sporegenerator.cpp \
code/worldobject/staticshape.cpp \
code/worldobject/teleport.cpp \
code/worldobject/tree.cpp \
code/worldobject/triffid.cpp \
code/worldobject/tripod.cpp \
code/worldobject/trunkport.cpp \
code/worldobject/upgrade_port.cpp \
code/worldobject/virii.cpp \
code/worldobject/wall.cpp \
code/worldobject/weapons.cpp \
code/worldobject/worldobject.cpp \
contrib/eclipse/eclbutton.cpp \
contrib/eclipse/eclwindow.cpp \
contrib/eclipse/eclipse.cpp \
contrib/netclass/ncsocklist.cpp \
contrib/netclass/ncsocket.cpp \
contrib/netclass/netclass.cpp \
contrib/netclass/ncmutex.cpp \
contrib/netclass/ncthread.cpp \
contrib/unrar/archive.cpp \
contrib/unrar/arcread.cpp \
contrib/unrar/cmddata.cpp \
contrib/unrar/consio.cpp \
contrib/unrar/crc.cpp \
contrib/unrar/crypt.cpp \
contrib/unrar/encname.cpp \
contrib/unrar/errhnd.cpp \
contrib/unrar/extinfo.cpp \
contrib/unrar/extract.cpp \
contrib/unrar/filcreat.cpp \
contrib/unrar/file.cpp \
contrib/unrar/filefn.cpp \
contrib/unrar/filestr.cpp \
contrib/unrar/find.cpp \
contrib/unrar/getbits.cpp \
contrib/unrar/global.cpp \
contrib/unrar/int64.cpp \
contrib/unrar/isnt.cpp \
contrib/unrar/list.cpp \
contrib/unrar/match.cpp \
contrib/unrar/options.cpp \
contrib/unrar/pathfn.cpp \
contrib/unrar/rarresource.cpp \
contrib/unrar/rarvm.cpp \
contrib/unrar/rarvmtbl.cpp \
contrib/unrar/rawread.cpp \
contrib/unrar/rdwrfn.cpp \
contrib/unrar/recvol.cpp \
contrib/unrar/rijndael.cpp \
contrib/unrar/rs.cpp \
contrib/unrar/savepos.cpp \
contrib/unrar/scantree.cpp \
contrib/unrar/sha1.cpp \
contrib/unrar/smallfn.cpp \
contrib/unrar/strfn.cpp \
contrib/unrar/strlist.cpp \
contrib/unrar/system.cpp \
contrib/unrar/timefn.cpp \
contrib/unrar/ulinks.cpp \
contrib/unrar/unicode.cpp \
contrib/unrar/unpack.cpp \
contrib/unrar/unrar.cpp \
contrib/unrar/volume.cpp \
contrib/binreloc/prefix.cpp

# Building 

DEMO_CPPFLAGS=-DTARGET_DEMOGAME=1
FULL_CPPFLAGS=-DTARGET_FULLGAME=1

OBJDIR_BASE=.linux-objs
DEMO_OBJDIR=$(OBJDIR_BASE)/demo
FULL_OBJDIR=$(OBJDIR_BASE)/full

ALL_OBJECTS=$(SOURCES:%.cpp=%.o)
DEMO_OBJECTS=$(ALL_OBJECTS:%.o=$(DEMO_OBJDIR)/%.o)
FULL_OBJECTS=$(ALL_OBJECTS:%.o=$(FULL_OBJDIR)/%.o)

all: darwinia.demo 

.linux-objs/demo/%.o: %.cpp
	@echo Compiling $<
	@mkdir -p $(dir $@)
	@$(CXX) $(DEMO_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.linux-objs/full/%.o: %.cpp
	@echo Compiling $<
	@mkdir -p $(dir $@)
	@$(CXX) $(FULL_CPPFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

darwinia.demo: $(DEMO_OBJECTS)
	@echo -n "Linking... "
	@$(LINK) -o $@ $+ $(LIBS) 

darwinia.full: $(FULL_OBJECTS)
	@echo -n "Linking... "
	@$(LINK) -o $@ $+ $(LIBS) 

clean:
	rm -f darwinia.demo darwinia.full
	rm -rf $(DEMO_OBJDIR) $(FULL_OBJDIR)

# Automatic Dependencies
DEPENDDIR_BASE = .linux-depend
DEPENDDIR_DEMO = $(DEPENDDIR_BASE)/demo
$(DEPENDDIR_DEMO)/%.d: %.cpp
	@echo make-depend $<
	@mkdir -p $(dir $@) 2> /dev/null
	@$(CC) -MM $(DEMO_CPPFLAGS) $(CPPFLAGS) $< | \
	( IFS=':' ; read target deps ; \
          echo $(DEMO_OBJDIR)/$(<:.cpp=.o) $@\:$$deps ) > $@

DEPENDDIR_FULL = $(DEPENDDIR_BASE)/full
$(DEPENDDIR_FULL)/%.d: %.cpp
	@echo make-depend $<
	@mkdir -p $(dir $@) 2> /dev/null
	@$(CC) -MM $(FULL_CPPFLAGS) $(CPPFLAGS) $< | \
	( IFS=':' ; read target deps ; \
          echo $(FULL_OBJDIR)/$(<:.cpp=.o) $@\:$$deps ) > $@

autodependencies = \
	$(patsubst %.cpp,$(DEPENDDIR_DEMO)/%.d,$(SOURCES)) \
	$(patsubst %.cpp,$(DEPENDDIR_FULL)/%.d,$(SOURCES))

-include $(autodependencies)



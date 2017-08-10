#include "physfs/physfs_file_system.hpp"
#include "supertux/menu/addon_gallery.hpp"
#include "util/file_system.hpp"
#include <physfs.h>

std::string addon_type_to_translated_string(Addon::Type type) {
  switch (type) {
  case Addon::LEVELSET:
    return _("Levelset");

  case Addon::WORLDMAP:
    return _("Worldmap");

  case Addon::WORLD:
    return _("World");

  case Addon::LANGUAGEPACK:
    return "";

  default:
    return _("Unknown");
  }
}

void AddonGallery::refresh() {
  clear();
  Downloader d;
  // If the Add-On is installed do not download, instead load locally
  try {
    std::vector<std::string> installedAddons =
        m_addon_manager->get_installed_addons();
    bool installed = (std::find(installedAddons.begin(), installedAddons.end(),
                                addon) != installedAddons.end());
    Addon &a = installed ? m_addon_manager->get_installed_addon(addon)
                         : m_addon_manager->get_repository_addon(addon);
    std::vector<std::pair<std::string, std::string>> availableScreenshots;
    // If installed mount the add-on
    std::string mountpoint;
    switch (a.get_format()) {
      case Addon::ORIGINAL:
        mountpoint = "";
        break;
      default:
        mountpoint = "custom/" + a.get_id();
        break;
    }
    bool mounted = installed;
    if (installed && !a.is_enabled())
      if (PHYSFS_mount(a.get_install_filename().c_str(), mountpoint.c_str(),
                       0) == 0) {
        log_warning << "Could not add " << a.get_install_filename()
                    << " to search path: " << PHYSFS_getLastError()
                    << std::endl;
        mounted = false;
      }
    for (size_t i = 0; i < a.get_screenshots().size(); i++) {
      try {
        if (!a.get_screenshots()[i].has_local() || !mounted) {
          try {
            d.download(a.get_screenshots()[i].get_url(),
                       boost::str(boost::format("screenshot-%d.png") % i));
            availableScreenshots.push_back(std::make_pair(
                boost::str(boost::format("screenshot-%d.png") % i),
                a.get_screenshots()[i].get_caption()));
          } catch (const std::exception &err) {
            log_debug << "Error when downloading: " << err.what() << std::endl;
          }
        } else {
          std::string path =
              FileSystem::join(FileSystem::join(mountpoint, "screenshots"),
                               a.get_screenshots()[i].get_local());
          log_debug << path << std::endl;
          if (PHYSFS_exists(path.c_str()))
          {
            availableScreenshots.push_back(
                std::make_pair(path, a.get_screenshots()[i].get_caption()));
          }else{
            log_debug << "Path to archive not found" << std::endl;
          }
        }
        log_debug << "Loaded screenshot " << i << std::endl;
      } catch (const std::exception &err) {
      }
    }
    add_label(a.get_title());
    add_hl();
    std::vector<SurfacePtr> images;
    std::vector<std::string> text;
    for (auto &p : availableScreenshots) {
      log_debug << "CREATING SURFACE " << std::endl;

      images.push_back(Surface::create(p.first));
      text.push_back(p.second);
    }
    // If the add-on had to be mounted because it was disabled, unmount it
    if (mounted && !a.is_enabled()) {
      PHYSFS_unmount(a.get_install_filename().c_str());
    }
    add_slideshow(images, text, 0);
    std::string type = addon_type_to_translated_string(a.get_type());
    add_keyvalue("Typ", "Weltkarte");
    add_keyvalue("Author", a.get_author());
    add_keyvalue("Rating",
                 (a.getRating() == -10)
                     ? "?/10"
                     : boost::str(boost::format("%d/10") % a.getRating()));
    add_keyvalue("Schwierigkeit",
                 (a.getDifficulty() == -10)
                     ? "?/10"
                     : boost::str(boost::format("%d/10") % a.getDifficulty()));
    add_keyvalue("Version", boost::str(boost::format("%d") % a.get_version()));
    if (installed) {
      try {
        Addon &possibleUpdate = m_addon_manager->get_repository_addon(addon);
        if (possibleUpdate.get_version() >= a.get_version() &&
            possibleUpdate.get_md5() != a.get_md5()) {
          add_entry(MN_ADDONGALLERY_UPDATE, _("Update"));
        } else {
          add_inactive(_("Update"));
        }
      } catch (const std::exception &err) {
        add_inactive(_("Update"));
      }

      if (a.is_enabled())
        add_entry(MN_ADDONGALLERY_DEACTIVATE, _("Deactivate"));
      else
        add_entry(MN_ADDONGALLERY_DEACTIVATE, _("Activate"));
    } else {
      add_entry(MN_ADDONGALLERY_INSTALL, _("Install"));
      add_inactive(_("Remove"));
    }
    add_back(_("Close"));
  } catch (const std::exception &err) {
    std::stringstream msg;
    msg << "Problem when loading gallery: " << err.what();
    throw std::runtime_error(msg.str());
  }
}

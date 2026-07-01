// This file Copyright © Mnemosaic LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosaic LLC.
// License text can be found in the licenses/ folder.

#include "PathButton.h"

#include "GtkCompat.h"
#include "Utils.h"

#include <giomm/file.h>
#include <glibmm/i18n.h>
#include <gtkmm/box.h>
#if GTKMM_CHECK_VERSION(4, 0, 0)
#include <glibmm/error.h>
#include <glibmm/property.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/filechoosernative.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/popover.h>
#include <gtkmm/separator.h>
#endif

#include <vector>

class PathButton::Impl
{
public:
    explicit Impl(PathButton& widget);
    Impl(Impl&&) = delete;
    Impl(Impl const&) = delete;
    Impl& operator=(Impl&&) = delete;
    Impl& operator=(Impl const&) = delete;
    ~Impl() = default;

#if GTKMM_CHECK_VERSION(4, 0, 0)
    std::string const& get_filename() const;
    void set_filename(std::string const& value);

    void set_recent_paths(std::vector<Glib::ustring> const& value);

    void add_filter(Glib::RefPtr<Gtk::FileFilter> const& value);

    Glib::Property<Gtk::FileChooser::Action>& property_action();
    Glib::Property<Glib::ustring>& property_title();

    sigc::signal<void()>& signal_selection_changed();
#endif

private:
#if GTKMM_CHECK_VERSION(4, 0, 0)
    void on_clicked();
    void populate_menu();
    void show_dialog();

    void update();
    void update_mode();
#endif

private:
#if GTKMM_CHECK_VERSION(4, 0, 0)
    PathButton& widget_;

    Glib::Property<Gtk::FileChooser::Action> action_;
    Glib::Property<Glib::ustring> title_;

    sigc::signal<void()> selection_changed_;

    Gtk::Image* const image_ = Gtk::make_managed<Gtk::Image>();
    Gtk::Label* const label_ = Gtk::make_managed<Gtk::Label>();
    Gtk::Image* const mode_ = Gtk::make_managed<Gtk::Image>();
    Gtk::Popover* const popover_ = Gtk::make_managed<Gtk::Popover>();

    std::string current_file_;
    std::vector<Glib::ustring> recent_paths_;
    std::vector<Glib::RefPtr<Gtk::FileFilter>> filters_;
#endif
};

PathButton::Impl::Impl([[maybe_unused]] PathButton& widget)
#if GTKMM_CHECK_VERSION(4, 0, 0)
    : widget_{ widget }
    , action_{ widget, "action", Gtk::FileChooser::Action::OPEN }
    , title_{ widget, "title", {} }
#endif
{
#if GTKMM_CHECK_VERSION(4, 0, 0)
    action_.get_proxy().signal_changed().connect([this]() { update_mode(); });

    label_->set_ellipsize(Pango::EllipsizeMode::END);
    label_->set_hexpand(true);
    label_->set_halign(Gtk::Align::START);

    auto* const layout = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
    layout->append(*image_);
    layout->append(*label_);
    layout->append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
    layout->append(*mode_);
    widget_.set_child(*layout);

    popover_->set_parent(widget_);
    popover_->set_position(Gtk::PositionType::BOTTOM);

    widget_.signal_clicked().connect(sigc::mem_fun(*this, &Impl::on_clicked));

    update();
    update_mode();
#endif
}

#if GTKMM_CHECK_VERSION(4, 0, 0)

std::string const& PathButton::Impl::get_filename() const
{
    return current_file_;
}

void PathButton::Impl::set_filename(std::string const& value)
{
    current_file_ = value;
    update();
    selection_changed_.emit();
}

void PathButton::Impl::set_recent_paths(std::vector<Glib::ustring> const& value)
{
    recent_paths_ = value;
}

void PathButton::Impl::add_filter(Glib::RefPtr<Gtk::FileFilter> const& value)
{
    filters_.push_back(value);
}

Glib::Property<Gtk::FileChooser::Action>& PathButton::Impl::property_action()
{
    return action_;
}

Glib::Property<Glib::ustring>& PathButton::Impl::property_title()
{
    return title_;
}

sigc::signal<void()>& PathButton::Impl::signal_selection_changed()
{
    return selection_changed_;
}

void PathButton::Impl::on_clicked()
{
    // When there are no recent paths to offer (e.g. file pickers), fall back to
    // opening the file chooser directly so the button behaves like a plain one.
    if (recent_paths_.empty()) {
        show_dialog();
        return;
    }

    populate_menu();
    popover_->popup();
}

void PathButton::Impl::populate_menu()
{
    auto* const box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);

    for (auto const& path : recent_paths_) {
        auto* const path_label = Gtk::make_managed<Gtk::Label>(path);
        path_label->set_halign(Gtk::Align::START);
        path_label->set_ellipsize(Pango::EllipsizeMode::START);
        path_label->set_max_width_chars(40);

        auto* const row = Gtk::make_managed<Gtk::Button>();
        row->set_child(*path_label);
        row->set_has_frame(false);
        row->set_tooltip_text(path);
        row->signal_clicked().connect([this, path = path.raw()]() {
            popover_->popdown();
            set_filename(path);
        });
        box->append(*row);
    }

    box->append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::HORIZONTAL));

    auto* const other_label = Gtk::make_managed<Gtk::Label>(_("Other…"));
    other_label->set_halign(Gtk::Align::START);

    auto* const other = Gtk::make_managed<Gtk::Button>();
    other->set_child(*other_label);
    other->set_has_frame(false);
    other->signal_clicked().connect([this]() {
        popover_->popdown();
        show_dialog();
    });
    box->append(*other);

    popover_->set_child(*box);
}

void PathButton::Impl::show_dialog()
{
    auto const title = title_.get_value();

    auto dialog = Gtk::FileChooserNative::create(
        !title.empty() ? title : _("Select a File"),
        action_.get_value(),
        _("_Open"),
        _("_Cancel"));
    dialog->set_transient_for(gtr_widget_get_window(widget_));
    dialog->set_modal(true);

    if (!current_file_.empty()) {
        dialog->set_file(Gio::File::create_for_path(current_file_));
    }

    for (auto const& filter : filters_) {
        dialog->add_filter(filter);
    }

    dialog->signal_response().connect([this, dialog](int response) mutable {
        if (response == TR_GTK_RESPONSE_TYPE(ACCEPT)) {
            set_filename(dialog->get_file()->get_path());
            selection_changed_.emit();
        }

        dialog.reset();
    });

    dialog->show();
}

void PathButton::Impl::update()
{
    if (!current_file_.empty()) {
        auto const file = Gio::File::create_for_path(current_file_);

        try {
            image_->set(file->query_info()->get_icon());
        } catch (Glib::Error const&) {
            image_->set_from_icon_name("image-missing");
        }

        label_->set_text(file->get_basename());
    } else {
        image_->set_from_icon_name("image-missing");
        label_->set_text(_("(None)"));
    }

    widget_.set_tooltip_text(current_file_);
}

void PathButton::Impl::update_mode()
{
    mode_->set_from_icon_name(
        action_.get_value() == Gtk::FileChooser::Action::SELECT_FOLDER ? "folder-open-symbolic" : "document-open-symbolic");
}

#endif

PathButton::PathButton()
    : Glib::ObjectBase(typeid(PathButton))
    , impl_(std::make_unique<Impl>(*this))
{
}

PathButton::PathButton(BaseObjectType* cast_item, Glib::RefPtr<Gtk::Builder> const& /*builder*/)
    : Glib::ObjectBase(typeid(PathButton))
    , BaseWidgetType(cast_item)
    , impl_(std::make_unique<Impl>(*this))
{
}

PathButton::~PathButton() = default;

void PathButton::set_recent_paths(std::vector<Glib::ustring> const& value)
{
#if GTKMM_CHECK_VERSION(4, 0, 0)
    impl_->set_recent_paths(value);
#else
    for (auto const& folder : value) {
        remove_shortcut_folder(folder.raw());
        add_shortcut_folder(folder.raw());
    }
#endif
}

#if GTKMM_CHECK_VERSION(4, 0, 0)

std::string PathButton::get_filename() const
{
    return impl_->get_filename();
}

void PathButton::set_filename(std::string const& value)
{
    impl_->set_filename(value);
}

void PathButton::add_filter(Glib::RefPtr<Gtk::FileFilter> const& value)
{
    impl_->add_filter(value);
}

Glib::PropertyProxy<Gtk::FileChooser::Action> PathButton::property_action()
{
    return impl_->property_action().get_proxy();
}

Glib::PropertyProxy<Glib::ustring> PathButton::property_title()
{
    return impl_->property_title().get_proxy();
}

sigc::signal<void()>& PathButton::signal_selection_changed()
{
    return impl_->signal_selection_changed();
}

#endif

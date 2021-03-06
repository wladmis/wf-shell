#include <gtkmm/window.h>
#include <glibmm/main.h>
#include <gdk/gdkwayland.h>
#include <config.hpp>
#include <animation.hpp>

#include <iostream>
#include <map>

#include <gtk-utils.hpp>
#include <wf-shell-app.hpp>
#include <wf-autohide-window.hpp>

#include "dock.hpp"

class WfDock::impl
{
    WayfireOutput *output;
    std::unique_ptr<WayfireAutohidingWindow> window;
    wl_surface *_wl_surface;

    Gtk::HBox box;

    public:
    impl(WayfireOutput *output)
    {
        this->output = output;
        window = std::unique_ptr<WayfireAutohidingWindow> (
            new WayfireAutohidingWindow(100, 100, output,
                ZWF_WM_SURFACE_V1_ROLE_PANEL));
        window->set_keyboard_mode(ZWF_WM_SURFACE_V1_KEYBOARD_FOCUS_MODE_NO_FOCUS);
        window->increase_autohide();

        window->set_position(WfDockApp::get().get_config()->get_section("dock")
            ->get_option("position", WF_WINDOW_POSITION_BOTTOM));

        _wl_surface = gdk_wayland_window_get_wl_surface(
            window->get_window()->gobj());

        window->signal_size_allocate().connect_notify(
            sigc::mem_fun(this, &WfDock::impl::on_allocation));
        window->add(box);
    }

    void add_child(Gtk::Widget& widget)
    {
        box.pack_end(widget);
        box.show_all();
    }

    void rem_child(Gtk::Widget& widget)
    {
        this->box.remove(widget);

        /* We now need to resize the dock so it fits the remaining widgets. */
        int total_width = 0;
        int total_height = last_height;
        box.foreach([&] (Gtk::Widget& child)
        {
            Gtk::Requisition min_req, pref_req;
            child.get_preferred_size(min_req, pref_req);

            total_width += min_req.width;
            total_height = std::max(total_height, min_req.height);
        });

        total_width = std::min(total_height, 100);
        this->window->resize(total_width, total_height);
        this->window->set_size_request(total_width, total_height);
    }

    wl_surface* get_wl_surface()
    {
        return this->_wl_surface;
    }

    int32_t last_width = 100, last_height = 100;
    void on_allocation(Gtk::Allocation& alloc)
    {
        if (last_width != alloc.get_width() || last_height != alloc.get_height())
        {
            last_width = alloc.get_width();
            last_height = alloc.get_height();
        }
    }
};

WfDock::WfDock(WayfireOutput *output)
    : pimpl(new impl(output)) { }
WfDock::~WfDock() = default;

void WfDock::add_child(Gtk::Widget& w) { return pimpl->add_child(w); }
void WfDock::rem_child(Gtk::Widget& w) { return pimpl->rem_child(w); }

wl_surface* WfDock::get_wl_surface() { return pimpl->get_wl_surface(); }

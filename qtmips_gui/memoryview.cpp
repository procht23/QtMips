#include "memoryview.h"

///////////////////////////
// Focus point (this is multiplied with height of widget to know position where we want to focus)
#define FOCUS 0.25
// How angle maps to pixels when and scroll is used
#define ANGLE_SCROLL 4
///////////////////////////

#include <iostream>
using namespace std;

MemoryView::MemoryView(QWidget *parent) : QWidget(parent) {
    memory = nullptr;
    addr_0 = 0;

    layout = new QVBoxLayout(this);

    memf= new Frame(this);
    layout->addWidget(memf);


    ctl_widg = new QWidget(this);
    layout->addWidget(ctl_widg);

    ctl_layout = new QHBoxLayout(ctl_widg);
    go_edit = new QLineEdit(ctl_widg);
    go_edit->setText("0x00000000");
    go_edit->setInputMask("\\0\\xHHHHHHHH");
    ctl_layout->addWidget(go_edit);
    connect(go_edit, SIGNAL(editingFinished()), this, SLOT(go_edit_finish()));
    up = new QToolButton(ctl_widg);
    up->setArrowType(Qt::UpArrow);
    connect(up, SIGNAL(clicked(bool)), this, SLOT(prev_section()));
    ctl_layout->addWidget(up);
    down = new QToolButton(ctl_widg);
    down->setArrowType(Qt::DownArrow);
    connect(down, SIGNAL(clicked(bool)), this, SLOT(next_section()));
    ctl_layout->addWidget(down);
}

void MemoryView::setup(machine::QtMipsMachine *machine) {
    memory = (machine == nullptr) ? nullptr : machine->memory();
    reload_content();
}

void MemoryView::set_focus(std::uint32_t address) {
    if (address < addr_0 || (address - addr_0)/4 > (unsigned)memf->widg->count()) {
        // This is outside of loaded area so just move it and reload everything
        // TODO this doesn't work with more than one column
        addr_0 = address - 4*memf->focussed_row();
        reload_content();
    } else {
        memf->focus_row((address - addr_0) / 4);
    }
}

std::uint32_t MemoryView::focus() {
    return addr_0 + 4*memf->focussed_row();
}

void MemoryView::edit_load_focus() {
    go_edit->setText(QString("0x%1").arg(focus(), 8, 16, QChar('0')));
}

void MemoryView::reload_content() {
    int count = memf->widg->count();
    memf->widg->clearRows();
    update_content(count, 0);
}

// This changes content to fit row_count number of rows including given number of columns
void MemoryView::resize_content(int row_count) {
    if (row_count > memf->widg->count()) { // insert

    } else { // remove

    }
}

void MemoryView::update_content(int count, int shift) {
    if (abs(shift) >= memf->widg->count()) {
        // This shifts more than we have so just reload whole content
        memf->widg->clearRows();
        addr_0 += 4*shift;
        for (int i = 0; i <= count; i++)
            memf->widg->addRow(row_widget(addr_0 + 4*i, memf->widg));
        return;
    }

    int d_b = shift;
    int d_e = count - memf->widg->count() - shift;

    if (d_b > 0)
        for (int i = 0; i < d_b; i++) {
            addr_0 -= 4;
            memf->widg->insertRow(row_widget(addr_0, memf->widg), 0);
        }
    else
        for (int i = 0; i > d_b; i--) {
            addr_0 += 4;
            memf->widg->removeRow(0);
        }
    if (d_e > 0)
        for (int i = 0; i < d_e; i++)
            memf->widg->addRow(row_widget(addr_0 + 4*memf->widg->count(), memf->widg));
    else
        for (int i = 0; i > d_e; i--)
            memf->widg->removeRow(memf->widg->count() - 1);
}

void MemoryView::go_edit_finish() {
    QString hex = go_edit->text();
    hex.remove(0, 2);

    bool ok;
    std::uint32_t nw = hex.toUInt(&ok, 16);
    if (ok) {
        set_focus(nw);
    } else
        edit_load_focus();
}

void MemoryView::next_section() {
    if (memory == nullptr)
        return;
    set_focus(memory->next_allocated(focus()));
}

void MemoryView::prev_section() {
    if (memory == nullptr)
        return;
    set_focus(memory->prev_allocated(focus()));
}

MemoryView::Frame::Frame(MemoryView *parent) : QAbstractScrollArea(parent) {
    mv = parent;
    focus = 0;

    widg = new StaticTable(this);
    setViewport(widg);

    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0, 0, 0, 0);
}

void MemoryView::Frame::resizeEvent(QResizeEvent *e) {
    QAbstractScrollArea::resizeEvent(e);
    mv->resize_content(widg->height() / widg->row_size());

    // TODO
    return;

    int row_h = widg->row_size();
    // Don't update if we are in limits
    if ((widg->y() < -hpart) && (widg->y() > -2*hpart) && (widg->height() >= req_height))
        return;

    // Calculate how many we need and how much we need to move and update content accordingly
    int count = (height() / row_h) + 1;
    int shift = (widg->y() + hpart + hpart/2)/row_h;
    mv->update_content(count * widg->columns(), shift * widg->columns());
    // Move and resize widget
    // Note count * row_h here is optimalization. We know row_h and count here so let's just use it
    widg->setGeometry(0, widg->y() - shift*row_h, width(), count * row_h);
    // Move focus to be consistent with shift we done
    focus += shift;
}

void MemoryView::Frame::wheelEvent(QWheelEvent *e) {
    QPoint pix = e->pixelDelta();
    QPoint ang = e->angleDelta();

    int shift;
    if (!pix.isNull())
        shift = pix.ry();
    else if (!ang.isNull())
        shift = ang.ry() * ANGLE_SCROLL;

    // Move focus by approriate amount but at least by one row
    mv->set_focus(mv->focus() + qMax(shift/widg->row_size(), (shift > -shift) - (shift < -shift)));
    // TODO update focus field rather in set_focus
    mv->edit_load_focus();
}

bool MemoryView::Frame::viewportEvent(QEvent *e) {
    bool p = QAbstractScrollArea::viewportEvent(e);
    // Pass paint event to viewport widget
    if (e->type() == QEvent::Paint)
        p = false;
    return p;
}

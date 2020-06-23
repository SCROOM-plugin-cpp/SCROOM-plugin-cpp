#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../src/pipette.hh"

BOOST_AUTO_TEST_SUITE(Pipette_Tests)

static int reg_sel = 0;
static int reg_post = 0;
static int tool_btn = 0;

class DummyView : public ViewInterface{
	void invalidate() {};
	ProgressInterface::Ptr getProgressInterface() { return nullptr; };
	void addSideWidget(std::string title, GtkWidget* w) {};
	void removeSideWidget(GtkWidget* w) {};
	void addToToolbar(GtkToolItem* ti) {};
	void removeFromToolbar(GtkToolItem* ti) {};
	void registerSelectionListener(SelectionListener::Ptr) { reg_sel++; };
	void registerPostRenderer(PostRenderer::Ptr) { reg_post++; };
	void setStatusMessage(const std::string&) {};
	boost::shared_ptr<PresentationInterface> getCurrentPresentation() { return nullptr; };
	void addToolButton(GtkToggleButton*, ToolStateListener::Ptr) { tool_btn++; };
};

BOOST_AUTO_TEST_CASE(view_add) {
	Pipette::Ptr pipette = Pipette::create();

	int pre_reg_sel = reg_sel;
	int pre_reg_post = reg_post;
    int pre_tool_btn = tool_btn;

    Scroom::Bookkeeping::Token token = pipette->viewAdded(ViewInterface::Ptr(new DummyView()));

    BOOST_CHECK(pre_reg_sel + 1 == reg_sel);
    BOOST_CHECK(pre_reg_post + 1 == reg_post);
    BOOST_CHECK(pre_tool_btn + 1 == tool_btn);
    BOOST_CHECK(token != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

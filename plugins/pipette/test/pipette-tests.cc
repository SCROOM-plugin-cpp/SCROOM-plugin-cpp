#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../src/pipette.hh"

BOOST_AUTO_TEST_SUITE(Pipette_Tests)

static int reg_sel = 0;
static int reg_post = 0;
static int tool_btn = 0;

class DummyView : public ViewInterface {
	void invalidate() {}
	ProgressInterface::Ptr getProgressInterface() { return nullptr; }
	void addSideWidget(std::string, GtkWidget*) {}
	void removeSideWidget(GtkWidget*) {}
	void addToToolbar(GtkToolItem*) {}
	void removeFromToolbar(GtkToolItem*) {}
	void registerSelectionListener(SelectionListener::Ptr) { reg_sel++; }
	void registerPostRenderer(PostRenderer::Ptr) { reg_post++; }
	void setStatusMessage(const std::string& msg) {
		//known text
		if (msg.compare("Computing color values...") || msg.compare("Pipette is not supported for this presentation.")) {
			return;
		}

		//actually part of the value_display test case
		if (msg.compare("Top-left: 10, Bottom-right: 11, Height: 12, Width: 13, Colors: C: 1.00")) {
			return;
		}

		BOOST_CHECK(false);
	}
	boost::shared_ptr<PresentationInterface> getCurrentPresentation() { return nullptr; }
	void addToolButton(GtkToggleButton*, ToolStateListener::Ptr) { tool_btn++; }
};

class DummyPresentation : public PresentationInterface, public PipetteViewInterface {
	Scroom::Utils::Rectangle<double> getRect() { return Scroom::Utils::Rectangle<int>(0, 0, 0, 0); }
	void redraw(ViewInterface::Ptr const&, cairo_t*, Scroom::Utils::Rectangle<double>, int) {}
	bool getProperty(const std::string& name, std::string&) { return false; }
	bool isPropertyDefined(const std::string&) { return false; }
	std::string getTitle() { return nullptr; }
	PipetteLayerOperations::PipetteColor getPixelAverages(Scroom::Utils::Rectangle<int>) {
        return {{"C", 1.0}};
	}
};

BOOST_AUTO_TEST_CASE(value_display) {
	PipetteHandler::Ptr handler = PipetteHandler::create();

	//test will fail via event handlers
    handler->computeValues(ViewInterface::Ptr(new DummyView), Scroom::Utils::Rectangle<int>(10, 11, 12, 13));
}

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

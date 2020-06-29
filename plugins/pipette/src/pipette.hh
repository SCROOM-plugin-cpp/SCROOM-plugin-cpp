#pragma once

#include <atomic>
#include <mutex>
#include <scroom/pipetteviewinterface.hh>
#include <scroom/plugininformationinterface.hh>
#include <scroom/threadpool.hh>
#include <scroom/utilities.hh>
#include <scroom/viewinterface.hh>

/**
 * Main handler for all pipette related events in a view. Manages
 * selections, the state of the plugin for a view and rendering
 * the current selection on top of a presentation.
 */
class PipetteHandler : public ToolStateListener,
                       public PostRenderer,
                       public SelectionListener,
                       virtual public Scroom::Utils::Base {
public:
  PipetteHandler();

public:
  typedef boost::shared_ptr<PipetteHandler> Ptr;

private:
  /**
   * Current selection.
   */
  Selection::Ptr selection;
  /**
   * Whether the plugin is currently enabled;
   */
  bool enabled;
  /**
   * Flag to indicate to the worker thread if the
   * computation result should be ignored because
   * the plugin was disabled.
   */
  std::atomic_flag wasDisabled;
  /**
   * Mutex to ensure only one background job can
   * exist at a time.
   */
  std::mutex jobMutex;
  /**
   * Thread queue for the active background job.
   */
  ThreadPool::Queue::Ptr currentJob;

public:
  /**
   * Creates a new PipetteHandler shared pointer and handler instance.
   */
  static Ptr create();

public:
  virtual ~PipetteHandler();

  ////////////////////////////////////////////////////////////////////////
  // PostRenderer

  void render(ViewInterface::Ptr const &vi, cairo_t *cr,
              Scroom::Utils::Rectangle<double> presentationArea,
              int zoom) override;

  ////////////////////////////////////////////////////////////////////////
  // SelectionListener

  void onSelectionStart(GdkPoint p, ViewInterface::Ptr view) override;
  void onSelectionUpdate(Selection::Ptr s, ViewInterface::Ptr view) override;
  void onSelectionEnd(Selection::Ptr s, ViewInterface::Ptr view) override;

  ////////////////////////////////////////////////////////////////////////
  // ToolStateListener

  void onEnable() override;
  void onDisable() override;

  ////////////////////////////////////////////////////////////////////////

  /**
   * Main function to compute all the average color values
   * for the current presentation.
   *
   * @param view The view to compute the average values for.
   * @param sel_rect The user selected presentation area.
   */
  void computeValues(ViewInterface::Ptr view,
                     Scroom::Utils::Rectangle<int> sel_rect);
  /**
   * Formats and sets the final status message with the pipette results.
   *
   * @param view The view to show the average values for.
   * @param rect The selected part of the presentation.
   * @param colors The average color values for the selected area.
   */
  void displayValues(ViewInterface::Ptr view,
                     Scroom::Utils::Rectangle<int> rect,
                     PipetteLayerOperations::PipetteColor colors);
};

/**
 * Main pipette plugin class. Manages all
 * registrations to the views that are opened.
 */
class Pipette : public PluginInformationInterface,
                public ViewObserver,
                virtual public Scroom::Utils::Base {
public:
  typedef boost::shared_ptr<Pipette> Ptr;

private:
  Pipette(){};

public:
  /**
   * Creates a new Pipette shared pointer and handler instance.
   */
  static Ptr create();

public:
  ////////////////////////////////////////////////////////////////////////
  // PluginInformationInterface

  std::string getPluginName() override;
  std::string getPluginVersion() override;
  void registerCapabilities(ScroomPluginInterface::Ptr host) override;

  ////////////////////////////////////////////////////////////////////////
  // ViewObserver

  Scroom::Bookkeeping::Token viewAdded(ViewInterface::Ptr v) override;

  ////////////////////////////////////////////////////////////////////////
};

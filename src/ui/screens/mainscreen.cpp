#include "mainscreen.h"

#include <cctype>
#include <memory>

#include "../../globalcontext.h"
#include "../../site.h"
#include "../../race.h"
#include "../../engine.h"
#include "../../sitemanager.h"
#include "../../site.h"
#include "../../sitelogic.h"
#include "../../sitelogicmanager.h"
#include "../../transferjob.h"
#include "../../util.h"
#include "../../remotecommandhandler.h"
#include "../../settingsloadersaver.h"
#include "../../preparedrace.h"
#include "../../sectionmanager.h"
#include "../../section.h"
#include "../../hourlyalltracking.h"

#include <cassert>

#include "../menuselectoptioncheckbox.h"
#include "../ui.h"
#include "../focusablearea.h"
#include "../menuselectadjustableline.h"
#include "../menuselectoptiontextbutton.h"
#include "../misc.h"

#include "allracesscreen.h"
#include "alltransferjobsscreen.h"

MainScreen::MainScreen(Ui * ui) {
  this->ui = ui;
}

void MainScreen::initialize(unsigned int row, unsigned int col) {
  baselegendtext = "[Down] Next option - [Up] Previous option - [A]dd site - [G]lobal settings - Event [l]og - [t]ransfers - All [r]aces - All transfer[j]obs - toggle [U]dp - Browse lo[c]al - General [i]nfo - [s]ections - [S]nake - [Esc] back to browsing";
  sitelegendtext = baselegendtext + " - [Tab] split browse - [right/b]rowse site - ra[w] command - [E]dit site - [C]opy site - [D]elete site - [q]uick jump - [L]ogin all slots - [0-9] Browse to section";
  preparelegendtext = baselegendtext + " - [Enter/s] start job - [Del] delete race";
  spreadjoblegendtext = baselegendtext + " - [Enter] Details - a[B]ort job - [T]ransfers for job - [R]eset job - [z] Abort job and delete own files on incomplete sites";
  transferjoblegendtext = baselegendtext + " - [Enter] Details - a[B]ort job - [T]ransfers for job";
  gotolegendtext = "[Any] Go to matching first letter in site list - [Esc] Cancel";
  autoupdate = true;
  gotomode = false;
  currentviewspan = 0;
  sitestartrow = 0;
  currentraces = 0;
  currenttransferjobs = 0;
  sitepos = 0;
  if (global->getEngine()->preparedRaces()) {
    focusedarea = &msop;
    msop.enterFocusFrom(0);
  }
  else if (global->getEngine()->currentRaces()) {
    focusedarea = &msosj;
    msosj.enterFocusFrom(0);
  }
  else if (global->getEngine()->currentTransferJobs()) {
    focusedarea = &msotj;
    msotj.enterFocusFrom(0);
  }
  else {
    focusedarea = &msos;
    msos.enterFocusFrom(0);
  }
  temphighlightline = -1;
  init(row, col);
}

void MainScreen::redraw() {
  ui->erase();
  ui->hideCursor();
  totalsitessize = global->getSiteManager()->getNumSites();
  if (sitepos && sitepos >= totalsitessize) {
    --sitepos;
  }
  numsitestext = "Sites: " + std::to_string(totalsitessize);
  int listpreparedraces = global->getEngine()->preparedRaces();
  int listraces = global->getEngine()->allRaces();
  int listtransferjobs = global->getEngine()->allTransferJobs();

  unsigned int irow = 0;
  msop.clear();
  msosj.clear();
  msotj.clear();
  msos.clear();
  if (listpreparedraces) {
    addPreparedRaceTableHeader(irow++, msop);
    std::list<std::shared_ptr<PreparedRace> >::const_iterator it;
    int i = 0;
    for (it = --global->getEngine()->getPreparedRacesEnd(); it != --global->getEngine()->getPreparedRacesBegin() && i < 3; it--, i++) {
      addPreparedRaceDetails(irow++, msop, *it);
    }
    msop.checkPointer();
    msop.adjustLines(col - 3);
    irow++;
  }
  if (listraces) {

    AllRacesScreen::addRaceTableHeader(irow++, msosj, std::string("SPREAD JOB NAME") + (listraces > 3 ? " (Showing latest 3)" : ""));
    std::list<std::shared_ptr<Race> >::const_iterator it;
    int i = 0;
    for (it = --global->getEngine()->getRacesEnd(); it != --global->getEngine()->getRacesBegin() && i < 3; it--, i++) {
      AllRacesScreen::addRaceDetails(irow++, msosj, *it);
    }
    msosj.checkPointer();
    msosj.adjustLines(col - 3);
    irow++;
  }
  if (listtransferjobs) {

    AllTransferJobsScreen::addJobTableHeader(irow++, msotj, std::string("TRANSFER JOB NAME") + (listtransferjobs > 3 ? " (Showing latest 3)" : ""));
    std::list<std::shared_ptr<TransferJob> >::const_iterator it;
    int i = 0;
    for (it = --global->getEngine()->getTransferJobsEnd(); it != --global->getEngine()->getTransferJobsBegin() && i < 3; it--, i++) {
      AllTransferJobsScreen::addJobDetails(irow++, msotj, *it);
    }
    msotj.checkPointer();
    msotj.adjustLines(col - 3);
    irow++;
  }
  msop.makeLeavableDown(listraces || listtransferjobs || totalsitessize);
  msosj.makeLeavableUp(listpreparedraces);
  msosj.makeLeavableDown(listtransferjobs || totalsitessize);
  msotj.makeLeavableUp(listpreparedraces || listraces);
  msotj.makeLeavableDown(totalsitessize);
  msos.makeLeavableUp(listpreparedraces || listraces || listtransferjobs);

  if (!totalsitessize) {
    ui->printStr(irow, 1, "Press 'A' to add a site");
  }
  else {
    int y = irow;
    addSiteHeader(y++, msos);
    sitestartrow = y;
    adaptViewSpan(currentviewspan, row - sitestartrow, sitepos, totalsitessize);
    for (unsigned int i = currentviewspan; (int)i < global->getSiteManager()->getNumSites() && i - currentviewspan < row - sitestartrow; ++i) {
      std::shared_ptr<Site> site = global->getSiteManager()->getSite(i);
      std::shared_ptr<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(site->getName());
      addSiteDetails(y++, msos, sl);
    }

    msos.setPointer(msos.getAdjustableLine(sitepos + 1 - currentviewspan)->getElement(0));
    msos.checkPointer();
    msos.adjustLines(col - 3);

    printSlider(ui, row, sitestartrow, col - 1, totalsitessize, currentviewspan);
  }
  int currentraces = global->getEngine()->currentRaces();
  int currenttransferjobs = global->getEngine()->currentTransferJobs();
  if (currentraces) {
    activeracestext = "Active spread jobs: " + std::to_string(currentraces) + "  ";
  }
  else {
    activeracestext = "";
  }
  if (currenttransferjobs) {
    activejobstext = "Active transfer jobs: " + std::to_string(currenttransferjobs) + "  ";
  }
  else {
    activejobstext = "";
  }
  if (focusedarea == &msop && !msop.size()) {
    msop.reset();
    focusedarea = &msosj;
    focusedarea->enterFocusFrom(0);
  }
  if (focusedarea == &msosj && !msosj.size()) {
    msosj.reset();
    focusedarea = &msotj;
    focusedarea->enterFocusFrom(0);
  }
  if (focusedarea == &msotj && !msotj.size()) {
    msotj.reset();
    focusedarea = &msos;
    focusedarea->enterFocusFrom(0);
  }
  printTable(msop);
  printTable(msosj);
  printTable(msotj);
  printTable(msos);
}

void MainScreen::printTable(MenuSelectOption & table) {
  if (temphighlightline != -1) {
    std::shared_ptr<MenuSelectAdjustableLine> highlightline = table.getAdjustableLineOnRow(temphighlightline);
    if (!!highlightline) {
      std::pair<unsigned int, unsigned int> minmaxcol = highlightline->getMinMaxCol();
      for (unsigned int i = minmaxcol.first; i <= minmaxcol.second; i++) {
        ui->printChar(temphighlightline, i, ' ', true);
      }
    }
  }
  for (unsigned int i = 0; i < table.size(); i++) {
    std::shared_ptr<ResizableElement> re = std::static_pointer_cast<ResizableElement>(table.getElement(i));
    bool highlight = false;
    if (table.isFocused() && (table.getSelectionPointer() == i || (int)re->getRow() == temphighlightline)) {
      highlight = true;
    }
    if (re->isVisible()) {
      ui->printStr(re->getRow(), re->getCol(), re->getLabelText(), highlight);
    }
  }
}

void MainScreen::update() {
  redraw();
  ui->setInfo();
}

void MainScreen::command(const std::string & command) {
  if (command == "yes") {
    if (!!abortrace) {
      global->getEngine()->abortRace(abortrace);
    }
    else if (!!abortjob) {
      global->getEngine()->abortTransferJob(abortjob);
    }
    else if (!!abortdeleterace) {
      global->getEngine()->deleteOnAllIncompleteSites(abortdeleterace, false);
    }
    else {
      global->getSiteLogicManager()->deleteSiteLogic(deletesite);
      global->getSiteManager()->deleteSite(deletesite);
      global->getSettingsLoaderSaver()->saveSettings();
    }
  }
  abortrace.reset();
  abortjob.reset();
  abortdeleterace.reset();
  ui->redraw();
  ui->setInfo();
}

void MainScreen::keyUp() {
  if (focusedarea == &msos) {
    if (sitepos) {
      --sitepos;
    }
    else if (msotj.size() || msosj.size() || msop.size()) {
      msos.defocus();
    }
  }
  else {
    focusedarea->goUp();
  }
  if (!focusedarea->isFocused()) {
    defocusedarea = focusedarea;
    if ((defocusedarea == &msos && !msotj.size() && !msosj.size()) ||
        (defocusedarea == &msotj && !msosj.size()) ||
        defocusedarea == &msosj)
    {
      focusedarea = &msop;
    }
    else if ((defocusedarea == &msos && !msotj.size()) ||
             defocusedarea == &msotj)
    {
      focusedarea = &msosj;
    }
    else {
      focusedarea = &msotj;
    }
    focusedarea->enterFocusFrom(2);
    ui->setLegend();
  }
}

void MainScreen::keyDown() {
  if (focusedarea == &msos) {
    if (sitepos + 1 < totalsitessize) {
      ++sitepos;
    }
  }
  else {
    focusedarea->goDown();
  }
  if (!focusedarea->isFocused()) {
    defocusedarea = focusedarea;
    if ((defocusedarea == &msop && !msosj.size() && !msotj.size()) ||
        (defocusedarea == &msosj && !msotj.size()) ||
        defocusedarea == &msotj)
    {
      focusedarea = &msos;
    }
    else if ((defocusedarea == &msop && !msosj.size()) ||
             defocusedarea == &msosj)
    {
      focusedarea = &msotj;
    }
    else {
      focusedarea = &msos;
    }
    focusedarea->enterFocusFrom(0);
    ui->setLegend();
  }
}

bool MainScreen::keyPressed(unsigned int ch) {
  if (temphighlightline != -1) {
    temphighlightline = -1;
    ui->redraw();
    if (ch == '-') {
      return true;
    }
  }
  unsigned int pagerows = (unsigned int) (row - sitestartrow) * 0.6;
  if (gotomode) {
    bool matched = false;
    if (ch >= 32 && ch <= 126) {
      for (int i = 0; i < global->getSiteManager()->getNumSites(); i++) {
        std::shared_ptr<Site> site = global->getSiteManager()->getSite(i);
        if (toupper(ch) == toupper(site->getName()[0])) {
          sitepos = i;
          matched = true;
          break;
        }
      }
    }
    gotomode = false;
    if (matched && !msos.isFocused()) {
      defocusedarea = focusedarea;
      defocusedarea->defocus();
      focusedarea = &msos;
      focusedarea->enterFocusFrom(0);
    }
    ui->update();
    ui->setLegend();
    return true;
  }
  switch(ch) {
    case '-': {
      MenuSelectOption * target = static_cast<MenuSelectOption *>(focusedarea);
      if (target && target->size() > 0) {
        temphighlightline = target->getElement(target->getSelectionPointer())->getRow();
        ui->redraw();
      }
      break;
    }
    case KEY_UP:
      keyUp();
      ui->redraw();
      return true;
    case KEY_DOWN:
      keyDown();
      ui->redraw();
      return true;
    case ' ':
    case 10:
      if (msos.isFocused()) {
        if (!msos.linesSize()) break;
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        std::shared_ptr<Site> site = global->getSiteManager()->getSite(sitename);
        ui->goSiteStatus(site->getName());
      }
      else if (msosj.isFocused() && msosj.size() > 0) {
        unsigned int id = msosj.getElement(msosj.getSelectionPointer())->getId();
        ui->goRaceStatus(id);
      }
      else if (msotj.isFocused()) {
        unsigned int id = msotj.getElement(msotj.getSelectionPointer())->getId();
        ui->goTransferJobStatus(id);
      }
      else if (msop.isFocused()) {
        unsigned int id = msop.getElement(msop.getSelectionPointer())->getId();
        global->getEngine()->startPreparedRace(id);
        ui->redraw();
      }
      return true;
    case 'G':
      ui->goGlobalOptions();
      return true;
    case 'i':
      ui->goInfo();
      return true;
    case 'l':
      ui->goEventLog();
      return true;
    case 'o':
      ui->goScoreBoard();
      return true;
    case 't':
      ui->goTransfers();
      return true;
    case 'r':
      ui->goAllRaces();
      return true;
    case 'A':
      ui->goAddSite();
      return true;
    case 's':
      if ((msop.isFocused())) {
        unsigned int id = msop.getElement(msop.getSelectionPointer())->getId();
        global->getEngine()->startPreparedRace(id);
        ui->redraw();
        return true;
      }
      ui->goSections();
      return true;
    case 'S':
      ui->goSnake();
      return true;
    case 'j':
      ui->goAllTransferJobs();
      return true;
    case 'U':
    {
      bool enabled = global->getRemoteCommandHandler()->isEnabled();
      global->getRemoteCommandHandler()->setEnabled(!enabled);
      return true;
    }
    case 'c':
      ui->goBrowseLocal();
      return true;
    case KEY_DC:
    case 'D':
      if (msos.isFocused()) {
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        std::shared_ptr<Site> site = global->getSiteManager()->getSite(sitename);
        if (!site) break;
        deletesite = site->getName();
        ui->goConfirmation("Do you really want to delete site " + deletesite);
      }
      else if (msop.isFocused()) {
        unsigned int id = msop.getElement(msop.getSelectionPointer())->getId();
        global->getEngine()->deletePreparedRace(id);
        ui->redraw();
      }
      return true;
    case 'B':
      if (msosj.isFocused() && msosj.size() > 0) {
        abortrace = global->getEngine()->getRace(msosj.getElement(msosj.getSelectionPointer())->getId());
        if (!!abortrace && abortrace->getStatus() == RaceStatus::RUNNING) {
          ui->goConfirmation("Do you really want to abort the spread job " + abortrace->getName());
        }
      }
      else if (msotj.isFocused() && msotj.size() > 0) {
        abortjob = global->getEngine()->getTransferJob(msotj.getElement(msotj.getSelectionPointer())->getId());
        if (!!abortjob && !abortjob->isDone()) {
          ui->goConfirmation("Do you really want to abort the transfer job " + abortjob->getName());
        }
      }
      return true;
    case 'z':
      if (msosj.isFocused() && msosj.size() > 0) {
        abortdeleterace = global->getEngine()->getRace(msosj.getElement(msosj.getSelectionPointer())->getId());
        if (!!abortdeleterace) {
          if (abortdeleterace->getStatus() == RaceStatus::RUNNING) {
            ui->goConfirmation("Do you really want to abort the race " + abortdeleterace->getName() + " and delete your own files on all incomplete sites?");
          }
          else {
            ui->goConfirmation("Do you really want to delete your own files in " + abortdeleterace->getName() + " on all involved sites?");
          }
        }
      }
      return true;
    case 'R':
      if (msosj.isFocused() && msosj.size() > 0) {
        std::shared_ptr<Race> race = global->getEngine()->getRace(msosj.getElement(msosj.getSelectionPointer())->getId());
        if (!!race) {
          global->getEngine()->resetRace(race, false);
        }
      }
      return true;
    case 'T':
      if (msosj.isFocused() && msosj.size() > 0) {
        std::shared_ptr<Race> race = global->getEngine()->getRace(msosj.getElement(msosj.getSelectionPointer())->getId());
        if (!!race) {
          ui->goTransfersFilterSpreadJob(race->getName());
        }
      }
      else if (msotj.isFocused() && msotj.size() > 0) {
        std::shared_ptr<TransferJob> tj = global->getEngine()->getTransferJob(msotj.getElement(msotj.getSelectionPointer())->getId());
        if (!!tj) {
          ui->goTransfersFilterTransferJob(tj->getName());
        }
      }
      return true;
    case KEY_NPAGE:
      for (unsigned int i = 0; i < pagerows; i++) {
        keyDown();
      }
      ui->redraw();
      return true;
    case KEY_PPAGE:
      for (unsigned int i = 0; i < pagerows; i++) {
        keyUp();
      }
      ui->redraw();
      return true;
    case 'q':
      gotomode = true;
      ui->update();
      ui->setLegend();
      return true;
    case 27: // esc
      ui->goContinueBrowsing();
      return true;
  }
  if (msos.isFocused()) {
    switch(ch) {
      case 'E': {
        if (!msos.linesSize()) break;
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        ui->goEditSite(sitename);
        return true;
      }
      case 'C': {
        if (!msos.linesSize()) break;
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        std::shared_ptr<Site> oldsite = global->getSiteManager()->getSite(sitename);
        std::shared_ptr<Site> site = std::make_shared<Site>(*oldsite);
        int i;
        for (i = 0; !!global->getSiteManager()->getSite(site->getName() + "-" + std::to_string(i)); i++);
        site->setName(site->getName() + "-" + std::to_string(i));
        global->getSiteManager()->addSite(site);
        global->getSettingsLoaderSaver()->saveSettings();
        ui->redraw();
        ui->setInfo();
        return true;
      }
      case 'b':
      case KEY_RIGHT: {
        if (!msos.linesSize()) break;
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        ui->goBrowse(sitename);
        return true;
      }
      case 'L': {
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        global->getSiteLogicManager()->getSiteLogic(sitename)->activateAll();
        return true;
      }
      case '\t': {
        if (!msos.linesSize()) break;
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        ui->goBrowseSplit(sitename);
        return true;
      }
      case 'w': {
        if (!msos.linesSize()) break;
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        ui->goRawCommand(sitename);
        return true;
      }
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        if (!msos.linesSize()) {
          break;
        }
        int hotkey = ch - '0';
        Section * section = global->getSectionManager()->getSection(hotkey);
        if (section == nullptr) {
          return true;
        }
        std::string sitename = msos.getElement(msos.getSelectionPointer())->getLabelText();
        std::shared_ptr<Site> site = global->getSiteManager()->getSite(sitename);
        if (site->hasSection(section->getName())) {
          Path path = site->getSectionPath(section->getName());
          ui->goBrowse(sitename, path);
        }
        return true;
      }
    }
  }
  return false;
}

std::string MainScreen::getLegendText() const {
  if (gotomode) {
    return gotolegendtext;
  }
  if (focusedarea == &msos) {
    return sitelegendtext;
  }
  if (focusedarea == &msop) {
    return preparelegendtext;
  }
  if (focusedarea == &msosj) {
    return spreadjoblegendtext;
  }
  if (focusedarea == &msotj) {
    return transferjoblegendtext;
  }
  return baselegendtext;
}

std::string MainScreen::getInfoLabel() const {
  return "CBFTP MAIN";
}

std::string MainScreen::getInfoText() const {
  std::string text;
  if (global->getRemoteCommandHandler()->isEnabled()) {
    text += "Remote commands enabled  ";
  }
  if (global->getEngine()->getNextPreparedRaceStarterEnabled()) {
    text += "Next spread job starter: ";
    if (global->getEngine()->getNextPreparedRaceStarterTimeout() != 0) {
      text += util::simpleTimeFormat(global->getEngine()->getNextPreparedRaceStarterTimeRemaining());
    }
    else {
      text += "Active";
    }
    text += "  ";
  }
  return text + activeracestext + activejobstext + numsitestext;
}

void MainScreen::addPreparedRaceTableRow(unsigned int y, MenuSelectOption & mso, unsigned int id,
    bool selectable, const std::string & section, const std::string & release, const std::string & ttl,
    const std::string & sites)
{
  std::shared_ptr<MenuSelectAdjustableLine> msal = mso.addAdjustableLine();
  std::shared_ptr<MenuSelectOptionTextButton> msotb;

  msotb = mso.addTextButtonNoContent(y, 1, "section", section);
  msotb->setSelectable(false);
  msal->addElement(msotb, 2, RESIZE_REMOVE);

  msotb = mso.addTextButton(y, 1, "release", release);
  if (!selectable) {
    msotb->setSelectable(false);
  }
  msotb->setId(id);
  msal->addElement(msotb, 4, 1, RESIZE_CUTEND, true);

  msotb = mso.addTextButtonNoContent(y, 1, "ttl", ttl);
  msotb->setSelectable(false);
  msal->addElement(msotb, 3, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "sites", sites);
  msotb->setSelectable(false);
  msal->addElement(msotb, 0, RESIZE_WITHDOTS);
}

void MainScreen::addPreparedRaceTableHeader(unsigned int y, MenuSelectOption & mso) {
  addPreparedRaceTableRow(y, mso, -1, false, "SECTION", "PREPARED SPREAD JOB NAME", "EXPIRES", "SITES");
}

void MainScreen::addPreparedRaceDetails(unsigned int y, MenuSelectOption & mso, const std::shared_ptr<PreparedRace> & preparedrace) {
  unsigned int id = preparedrace->getId();
  std::string section = preparedrace->getSection();
  std::string release = preparedrace->getRelease();
  std::string ttl = util::simpleTimeFormat(preparedrace->getRemainingTime());
  std::list<std::string> sites = preparedrace->getSites();
  std::string sitestr;
  for (std::list<std::string>::const_iterator it = sites.begin(); it != sites.end(); it++) {
    sitestr += *it + ",";
  }
  if (sitestr.length() > 0) {
    sitestr = sitestr.substr(0, sitestr.length() - 1);
  }
  addPreparedRaceTableRow(y, mso, id, true, section, release, ttl, sitestr);
}

void MainScreen::addSiteHeader(unsigned int y, MenuSelectOption & mso) {
  addSiteRow(y, mso, false, "SITE           ", "LOGINS", "UPLOADS", "DOWNLOADS", "UP", "DOWN", "DISABLED", "UP 24HR", "DOWN 24HR", "ALLUP", "ALLDOWN", "PRIORITY");
}

void MainScreen::addSiteRow(unsigned int y, MenuSelectOption & mso, bool selectable, const std::string & site,
    const std::string & logins, const std::string & uploads, const std::string & downloads,
    const std::string & allowup, const std::string & allowdown, const std::string & disabled,
    const std::string & dayup, const std::string & daydn, const std::string & alup, const std::string & aldn,
    const std::string & prio)
{
  std::shared_ptr<MenuSelectAdjustableLine> msal = mso.addAdjustableLine();
  std::shared_ptr<MenuSelectOptionTextButton> msotb;

  msotb = mso.addTextButtonNoContent(y, 1, "site", site);
  if (!selectable) {
    msotb->setSelectable(false);
  }
  msal->addElement(msotb, 13, 6, RESIZE_CUTEND, false);

  msotb = mso.addTextButtonNoContent(y, 1, "logins", logins);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 12, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "uploads", uploads);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 10, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "downloads", downloads);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 11, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "up", allowup);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 8, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "down", allowdown);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 9, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "disabled", disabled);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 7, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "dayup", dayup);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 2, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "daydn", daydn);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 3, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "alup", alup);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 4, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "aldn", aldn);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 5, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "prio", prio);
  msotb->setSelectable(false);
  msotb->setRightAligned();
  msal->addElement(msotb, 1, RESIZE_REMOVE);
}

void MainScreen::addSiteDetails(unsigned int y, MenuSelectOption & mso, const std::shared_ptr<SiteLogic> & sl) {
  std::shared_ptr<Site> site = sl->getSite();
  std::string sitename = site->getName();
  std::string logins = std::to_string(sl->getCurrLogins());
  if (!site->unlimitedLogins()) {
    logins += "/" + std::to_string(site->getMaxLogins());
  }
  std::string uploads = std::to_string(sl->getCurrUp());
  if (!site->unlimitedUp()) {
    uploads += "/" + std::to_string(site->getMaxUp());
  }
  std::string downloads = std::to_string(sl->getCurrDown());
  if (!site->unlimitedDown()) {
    downloads += "/" + std::to_string(site->getMaxDown());
  }
  std::string up;
  switch (site->getAllowUpload()) {
    case SITE_ALLOW_TRANSFER_NO:
      up = "[ ]";
      break;
    case SITE_ALLOW_TRANSFER_YES:
      up = "[X]";
      break;
    default:
      assert(false);
      break;
  }
  std::string down;
  switch (site->getAllowDownload()) {
    case SITE_ALLOW_TRANSFER_NO:
      down = "[ ]";
      break;
    case SITE_ALLOW_TRANSFER_YES:
      down = "[X]";
      break;
    case SITE_ALLOW_DOWNLOAD_MATCH_ONLY:
      down = "[A]";
      break;
  }
  std::string disabled = site->getDisabled()? "[X]" : "[ ]";
  std::string up24 = util::parseSize(site->getSizeUp().getLast24Hours());
  std::string down24 = util::parseSize(site->getSizeDown().getLast24Hours());
  std::string allup = util::parseSize(site->getSizeUp().getAll());
  std::string alldown = util::parseSize(site->getSizeDown().getAll());
  std::string prio = Site::getPriorityText(site->getPriority());
  addSiteRow(y, mso, true, sitename, logins, uploads, downloads, up, down, disabled, up24, down24, allup, alldown, prio);
}

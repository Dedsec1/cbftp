#include "scoreboardelement.h"

#include "race.h"

ScoreBoardElement::ScoreBoardElement(const std::string & filename, unsigned short score,
    unsigned long long int filesize, PrioType priotype, const std::shared_ptr<SiteLogic> & src, FileList * fls, SiteRace * srcsr,
    const std::shared_ptr<SiteLogic> & dst, FileList * fld, SiteRace * dstsr, const std::shared_ptr<Race> & race,
    const std::string & subdir)
{
  reset(filename, score, filesize, priotype, src, fls, srcsr, dst, fld, dstsr, race, subdir);
}

void ScoreBoardElement::reset(const std::string & filename, unsigned short score,
    unsigned long long int filesize, PrioType priotype, const std::shared_ptr<SiteLogic> & src, FileList * fls, SiteRace * srcsr,
    const std::shared_ptr<SiteLogic> & dst, FileList * fld, SiteRace * dstsr, const std::shared_ptr<Race> & race,
    const std::string & subdir)
{
  this->filename = filename;
  this->src = src;
  this->fls = fls;
  this->srcsr = srcsr;
  this->dst = dst;
  this->fld = fld;
  this->dstsr = dstsr;
  this->race = race;
  this->score = score;
  this->priotype = priotype;
  attempted = false;
  this->subdir = subdir;
  this->filesize = filesize;
  skipchecked = true;
}

void ScoreBoardElement::reset(const ScoreBoardElement & other) {
  reset(other.filename, other.score, other.filesize, other.priotype, other.src, other.fls,
        other.srcsr, other.dst, other.fld, other.dstsr, other.race,
        other.subdir);
}

void ScoreBoardElement::update(unsigned short score, bool unsetattempted) {
  this->score = score;
  if (unsetattempted) {
    attempted = false;
  }
}

const std::string & ScoreBoardElement::fileName() const {
  return filename;
}

const std::string & ScoreBoardElement::subDir() const {
  return subdir;
}

const std::shared_ptr<SiteLogic> & ScoreBoardElement::getSource() const {
  return src;
}

const std::shared_ptr<SiteLogic> & ScoreBoardElement::getDestination() const {
  return dst;
}

FileList * ScoreBoardElement::getSourceFileList() const {
  return fls;
}

FileList * ScoreBoardElement::getDestinationFileList() const {
  return fld;
}

SiteRace * ScoreBoardElement::getSourceSiteRace() const {
  return srcsr;
}

SiteRace * ScoreBoardElement::getDestinationSiteRace() const {
  return dstsr;
}

const std::shared_ptr<Race> & ScoreBoardElement::getRace() const {
  return race;
}

unsigned short ScoreBoardElement::getScore() const {
  return score;
}

PrioType ScoreBoardElement::getPriorityType() const {
  return priotype;
}

unsigned long long int ScoreBoardElement::getFileSize() const {
  return filesize;
}

bool ScoreBoardElement::wasAttempted() const {
  return attempted;
}

void ScoreBoardElement::setAttempted() {
  attempted = true;
}

bool ScoreBoardElement::skipChecked() const {
  return skipchecked;
}

void ScoreBoardElement::setSkipChecked() {
  skipchecked = true;
}

void ScoreBoardElement::resetSkipChecked() {
  skipchecked = false;
}

std::ostream & operator<<(std::ostream & out, const ScoreBoardElement & sbe) {
  return out << sbe.fileName() << " - " << sbe.getScore();
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "PresetManager.h"
#include "Parameters.h"

namespace jane60
{

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state, const std::vector<FactoryPatch>& factory)
    : state_ (state), factory_ (factory)
{
    rescan();
    if (! factory_.empty())
    {
        // Start on the first factory patch for real: the parameter defaults are not a patch, and
        // leaving them made the strip show "edited" at launch before anything was touched.
        apply (factory_.front().panel);
        other_ = loaded_;
    }
}

juce::File PresetManager::userFolder()
{
   #if JUCE_MAC
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
        .getChildFile ("Library/Audio/Presets/clevergear/Jane-Sixty");
   #elif JUCE_WINDOWS
    // %APPDATA%\clevergear\Jane-Sixty
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("clevergear").getChildFile ("Jane-Sixty");
   #else
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("clevergear").getChildFile ("Jane-Sixty");
   #endif
}

void PresetManager::rescan()
{
    entries_.clear();
    for (std::size_t i = 0; i < factory_.size(); ++i)
    {
        Entry e;
        e.name = juce::String (factory_[i].number) + " " + factory_[i].name;
        e.bank = "Factory";
        e.factoryIndex = static_cast<int> (i);
        entries_.push_back (e);
    }
    const auto folder = userFolder();
    if (folder.isDirectory())
    {
        juce::Array<juce::File> files;
        folder.findChildFiles (files, juce::File::findFiles, true, "*.json");
        files.sort();
        for (const auto& f : files)
        {
            Entry e;
            e.name = f.getFileNameWithoutExtension();
            e.bank = f.getParentDirectory() == folder ? "User" : f.getParentDirectory().getFileName();
            e.file = f;
            entries_.push_back (e);
        }
    }
    if (current_ >= static_cast<int> (entries_.size()))
        current_ = 0;
}

juce::String PresetManager::currentName() const
{
    if (current_ < 0 || current_ >= static_cast<int> (entries_.size())) return {};
    return entries_[static_cast<std::size_t> (current_)].name;
}

PanelState PresetManager::snapshot() const
{
    return params::readPanel (state_);
}

void PresetManager::apply (const PanelState& p)
{
    params::writePanel (state_, p);
    loaded_ = p;
}

bool PresetManager::isEdited() const
{
    return ! patchFieldsEqual (snapshot(), loaded_);
}

void PresetManager::load (int index)
{
    if (index < 0 || index >= static_cast<int> (entries_.size())) return;
    const Entry& e = entries_[static_cast<std::size_t> (index)];
    if (e.factoryIndex >= 0)
    {
        apply (factory_[static_cast<std::size_t> (e.factoryIndex)].panel);
    }
    else
    {
        try
        {
            const auto p = presetFromJson (e.file.loadFileAsString().toStdString());
            apply (p.panel);
        }
        catch (const std::exception&)
        {
            return;
        }
    }
    current_ = index;
}

void PresetManager::loadNext()
{
    if (entries_.empty()) return;
    load ((current_ + 1) % static_cast<int> (entries_.size()));
}

void PresetManager::loadPrevious()
{
    if (entries_.empty()) return;
    load ((current_ + static_cast<int> (entries_.size()) - 1) % static_cast<int> (entries_.size()));
}

juce::Result PresetManager::saveAs (const juce::String& name, const juce::String& bank, const juce::StringArray& tags)
{
    if (name.trim().isEmpty())
        return juce::Result::fail ("Preset needs a name");
    auto folder = userFolder();
    if (bank.isNotEmpty() && bank != "User")
        folder = folder.getChildFile (juce::File::createLegalFileName (bank));
    if (! folder.createDirectory())
        return juce::Result::fail ("Cannot create " + folder.getFullPathName());

    Preset p;
    p.name = name.toStdString();
    p.bank = bank.isEmpty() ? "User" : bank.toStdString();
    for (const auto& t : tags) p.tags.push_back (t.toStdString());
    p.panel = snapshot();

    const auto file = folder.getChildFile (juce::File::createLegalFileName (name) + ".json");
    if (! file.replaceWithText (presetToJson (p)))
        return juce::Result::fail ("Cannot write " + file.getFullPathName());

    loaded_ = p.panel;
    rescan();
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].file == file) current_ = static_cast<int> (i);
    return juce::Result::ok();
}

juce::Result PresetManager::importFile (const juce::File& jsonFile)
{
    try
    {
        const auto p = presetFromJson (jsonFile.loadFileAsString().toStdString());
        apply (p.panel);
        return juce::Result::ok();
    }
    catch (const std::exception& e)
    {
        return juce::Result::fail (e.what());
    }
}

juce::Result PresetManager::exportCurrent (const juce::File& jsonFile, const juce::String& name) const
{
    Preset p;
    p.name = name.toStdString();
    p.bank = "User";
    p.panel = snapshot();
    if (! jsonFile.replaceWithText (presetToJson (p)))
        return juce::Result::fail ("Cannot write " + jsonFile.getFullPathName());
    return juce::Result::ok();
}

void PresetManager::toggleAB()
{
    const PanelState active = snapshot();
    const PanelState recall = other_;
    other_ = active;
    slotB_ = ! slotB_;
    params::writePanel (state_, recall);
    loaded_ = recall;
}

void PresetManager::copyActiveToOther()
{
    other_ = snapshot();
}

void PresetManager::writeTo (juce::ValueTree& tree) const
{
    tree.setProperty ("presetIndex", current_, nullptr);
    tree.setProperty ("presetName", currentName(), nullptr);
    tree.setProperty ("abSlotB", slotB_, nullptr);
    Preset o;
    o.name = "other";
    o.panel = other_;
    tree.setProperty ("abOther", juce::String (presetToJson (o)), nullptr);
    Preset l;
    l.name = "loaded";
    l.panel = loaded_;
    tree.setProperty ("loaded", juce::String (presetToJson (l)), nullptr);
}

void PresetManager::readFrom (const juce::ValueTree& tree)
{
    current_ = static_cast<int> (tree.getProperty ("presetIndex", 0));
    slotB_ = static_cast<bool> (tree.getProperty ("abSlotB", false));
    try
    {
        if (tree.hasProperty ("abOther"))
            other_ = presetFromJson (tree["abOther"].toString().toStdString()).panel;
        if (tree.hasProperty ("loaded"))
            loaded_ = presetFromJson (tree["loaded"].toString().toStdString()).panel;
    }
    catch (const std::exception&)
    {
    }
    rescan();
}

} // namespace jane60

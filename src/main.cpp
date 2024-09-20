#include <Geode/Geode.hpp>
#include <Geode/modify/SetGroupIDLayer.hpp>
#include <Geode/modify/CCScrollLayerExt.hpp>

using namespace geode::prelude;

class LimitedCCMenu : public CCMenu {

	public:

	geode::ScrollLayer* m_scrollLayer;

	static LimitedCCMenu* create() {
		auto ret = new LimitedCCMenu();
		if (ret->init()) {
			ret->autorelease();
			return ret;
		}

		delete ret;
		return nullptr;
	}

	bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {

		if (m_scrollLayer) {
			CCSize scrollSize = m_scrollLayer->getScaledContentSize();
			CCPoint anchorPoint = m_scrollLayer->getAnchorPoint();

			float startPointX = m_scrollLayer->getPositionX() - scrollSize.width * anchorPoint.x;
			float startPointY = m_scrollLayer->getPositionY() - scrollSize.height * anchorPoint.y;

			CCRect rect = {startPointX, startPointY, scrollSize.width, scrollSize.height};

			if (rect.containsPoint(touch->getLocation())) {
				return CCMenu::ccTouchBegan(touch, event);
			}
		}
		else {
			return CCMenu::ccTouchBegan(touch, event);
		}
		return false;
	}
};

class $modify(MySetGroupIDLayer, SetGroupIDLayer) {

	struct GroupData {
		std::vector<int> groups;
		std::vector<int> parentGroups;
		GameObject* object;
	};

	struct Fields {
		Ref<geode::ScrollLayer> m_scrollLayer;
		Ref<CCMenu> m_currentMenu;
		int m_lastRemoved = 0;
		float m_scrollPos = INT_MIN;
	};

    bool init(GameObject* obj, cocos2d::CCArray* objs) {
		if (!SetGroupIDLayer::init(obj, objs)) {
			return false;
		}

		if (CCNode* node = m_mainLayer->getChildByID("groups-list-menu")) {
			node->setVisible(false);
		}

		if (CCNode* node = m_mainLayer->getChildByID("add-group-id-buttons-menu")) {
			
			if (CCMenuItemSpriteExtra* idBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(node->getChildByID("add-group-id-button"))) {
				idBtn->m_pfnSelector = menu_selector(MySetGroupIDLayer::onAddGroup2);
			}

			if (CCMenuItemSpriteExtra* parentBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(node->getChildByID("add-group-parent-button"))) {
				parentBtn->m_pfnSelector = menu_selector(MySetGroupIDLayer::onAddGroupParent2);
			}
		}


		regenerateGroupView();
		return true;
	}

	void onRemoveFromGroup2(CCObject* obj) {
		m_fields->m_lastRemoved = obj->getTag();
		SetGroupIDLayer::onRemoveFromGroup(obj);
		regenerateGroupView();
	}

	void onAddGroup2(CCObject* obj) {
		SetGroupIDLayer::onAddGroup(obj);
		regenerateGroupView();
	}

	void onAddGroupParent2(CCObject* obj) {
		SetGroupIDLayer::onAddGroupParent(obj);
		regenerateGroupView();
	}

	void regenerateGroupView() {
		if (m_fields->m_scrollLayer) m_fields->m_scrollLayer->removeFromParent();

		std::vector<GroupData> groupData;
		std::map<int, int> allGroups;
		std::map<int, int> allParentGroups;

		if (!m_targetObjects || m_targetObjects->count() == 0) {
			groupData.push_back(parseObjGroups(m_targetObject));
		}
		else {
			for (GameObject* obj : CCArrayExt<GameObject*>(m_targetObjects)) {
				groupData.push_back(parseObjGroups(obj));
			}
		}

		for(GroupData data : groupData) {
			for (int group : data.groups) {
				allGroups[group]++;
			}
			for (int group : data.parentGroups) {
				allParentGroups[group]++;
			}
		}

		allGroups.erase(0);
		allParentGroups.erase(0);

		if (allParentGroups.count(m_fields->m_lastRemoved)) {
			allParentGroups.erase(m_fields->m_lastRemoved);
		}
		else {
			allGroups.erase(m_fields->m_lastRemoved);
		}

		CCNode* menuContainer = CCNode::create();
		LimitedCCMenu* groupsMenu = LimitedCCMenu::create();

		RowLayout* layout = RowLayout::create();
		layout->setGap(12);
		layout->setAutoScale(false);
		layout->setGrowCrossAxis(true);
		layout->setCrossAxisOverflow(true);

		groupsMenu->setLayout(layout);

		m_fields->m_lastRemoved = 0;

		for (auto [k, v] : allGroups) {
			bool isParent = allParentGroups.count(k);
			bool isAlwaysPresent = v == groupData.size();

			std::string texture = "GJ_button_04.png";

			if (!isAlwaysPresent) texture = "GJ_button_05.png";
			if (isParent) texture = "GJ_button_03.png";

			ButtonSprite* bspr = ButtonSprite::create(fmt::format("{}", k).c_str(), 30, true, "goldFont.fnt", texture.c_str(), 20, 0.5);

			CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(bspr, this, menu_selector(MySetGroupIDLayer::onRemoveFromGroup2));
			
			button->setTag(k);
			
			groupsMenu->addChild(button);
		}
		CCSize contentSize;

		if (groupsMenu->getChildrenCount() <= 10) {
			groupsMenu->setScale(1.f);
			contentSize = CCSize{290, 50};
		}
		else {
			groupsMenu->setScale(0.85f);
			contentSize = CCSize{400, 50};
		}

		float padding = 7.5;

		groupsMenu->setContentSize(contentSize);
		groupsMenu->setPosition({360/2, padding});
		groupsMenu->setAnchorPoint({0.5, 0});
		groupsMenu->updateLayout();

		m_fields->m_currentMenu = groupsMenu;
		menuContainer->setContentSize({360, groupsMenu->getScaledContentSize().height + padding * 2});
		menuContainer->setAnchorPoint({0.5, 0});
		menuContainer->setPosition({360/2, 0});
		menuContainer->addChild(groupsMenu);

		CCSize winSize = CCDirector::get()->getWinSize();

		m_fields->m_scrollLayer = ScrollLayer::create({360, menuContainer->getScaledContentSize().height});
		m_fields->m_scrollLayer->setContentSize({360, 68});
		m_fields->m_scrollLayer->setPosition({winSize.width/2, winSize.height/2 - 16.8f});
		m_fields->m_scrollLayer->ignoreAnchorPointForPosition(false);
		m_fields->m_scrollLayer->m_contentLayer->addChild(menuContainer);
		m_fields->m_scrollLayer->setID("groups-list-menu-scroll"_spr);
		groupsMenu->m_scrollLayer = m_fields->m_scrollLayer;

		m_mainLayer->addChild(m_fields->m_scrollLayer);
		if (m_fields->m_scrollPos == INT_MIN) {
			m_fields->m_scrollLayer->scrollToTop();
		}
		else {
			float minY = -(m_fields->m_scrollLayer->m_contentLayer->getContentSize().height - m_fields->m_scrollLayer->getContentSize().height);
			float pos = m_fields->m_scrollPos;

			log::info("minY: {}", minY);
			log::info("posY: {}", m_fields->m_scrollPos);

			if (m_fields->m_scrollPos < minY) pos = minY;
			if (m_fields->m_scrollPos > 0) pos = 0;

			m_fields->m_scrollLayer->m_contentLayer->setPositionY(pos);
		}

		if (groupsMenu->getChildrenCount() <= 14) {
			m_fields->m_scrollLayer->m_disableMovement = true;
			m_fields->m_scrollLayer->enableScrollWheel(false);
		}

		m_mainLayer->removeChildByID("group-count"_spr);

		CCLabelBMFont* groupCountLabel = CCLabelBMFont::create(fmt::format("Groups: {}", allGroups.size()).c_str(), "chatFont.fnt");

		if (CCNode* zLayerLabel = m_mainLayer->getChildByID("z-layer-label")) {
			if (CCNode* groupsBG = m_mainLayer->getChildByID("groups-bg")) {
				CCPoint labelPos = zLayerLabel->getPosition();
				CCSize groupsBGSize = groupsBG->getContentSize();
				groupCountLabel->setPosition({labelPos.x - groupsBGSize.width/2, labelPos.y + 6});
			}
		}
		groupCountLabel->setID("group-count"_spr);
		groupCountLabel->setAnchorPoint({0, 0.5});
		groupCountLabel->setColor({0, 0, 0});
		groupCountLabel->setOpacity(200);
		groupCountLabel->setScale(0.5f);
		m_mainLayer->addChild(groupCountLabel);

		handleTouchPriority(this);

		queueInMainThread([this] {
			if (auto delegate = typeinfo_cast<CCTouchDelegate*>(m_fields->m_scrollLayer.data())) {
				if (auto handler = CCTouchDispatcher::get()->findHandler(delegate)) {
					CCTouchDispatcher::get()->setPriority(handler->getPriority() - 1, handler->getDelegate());
				}
			}
		});
	}

	GroupData parseObjGroups(GameObject* obj) {

		LevelEditorLayer* lel = LevelEditorLayer::get();

		int uuid = obj->m_uniqueID;
		std::vector<int> parents;

		for (auto [k, v] : CCDictionaryExt<int, CCArray*>(lel->m_unknownE40)) {
			if (k == uuid) {
				for (auto val : CCArrayExt<CCInteger*>(v)) {
					parents.push_back(val->getValue());
				}
			}
		}

		std::vector<int> groups;

		if (obj->m_groups) {
			groups = std::vector<int>{obj->m_groups->begin(), obj->m_groups->end()};
		}

		return GroupData{groups, parents, obj};
	}

	static MySetGroupIDLayer* get() {

		CCScene* scene = CCDirector::get()->getRunningScene();
		return static_cast<MySetGroupIDLayer*>(getChildOfType<SetGroupIDLayer>(scene, 0));
	}

	void setButtonsEnabled(bool enabled) {
		if (CCMenu* menu = m_fields->m_currentMenu) {
			for (CCMenuItemSpriteExtra* btn : CCArrayExt<CCMenuItemSpriteExtra*>(menu->getChildren())) {
				btn->setEnabled(enabled);
				btn->stopAllActions();
				btn->setScale(1);
			}
		}
	}
};


// cheaty way to ensure groups don't accidentally get deleted, if you see this and wanna do better, please go for it :3

class $modify(CCScrollLayerExt) {

    void ccTouchMoved(cocos2d::CCTouch* p0, cocos2d::CCEvent* p1) {
		CCScrollLayerExt::ccTouchMoved(p0, p1);

		if (auto groupIDLayer = MySetGroupIDLayer::get()) {
			float dY = std::abs(p0->getStartLocation().y - p0->getLocation().y);
			if (dY > 3) {
				groupIDLayer->setButtonsEnabled(false);
			}
		}
	}

    void ccTouchEnded(cocos2d::CCTouch* p0, cocos2d::CCEvent* p1) {
		CCScrollLayerExt::ccTouchEnded(p0, p1);
		
		if (auto groupIDLayer = MySetGroupIDLayer::get()) {
			groupIDLayer->m_fields->m_scrollPos = m_contentLayer->getPositionY();
			groupIDLayer->setButtonsEnabled(true);
		}
	}
};
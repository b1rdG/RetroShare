#include <QObject>
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxstunnel.h>
#include <retroshare/rsservicecontrol.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsdistsync.h>
#include "TurtleRouterDialog.h"
#include <QPainter>
#include <QStylePainter>
#include <algorithm> // for sort

#include "gui/settings/rsharesettings.h"

static const uint MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;


TurtleRouterDialog::TurtleRouterDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	// Init the basic setup.
	//
	QStringList stl ;
	int n=0 ;

	stl.clear() ;
	stl.push_back(tr("Search requests")) ;
	top_level_s_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_s_requests) ;

	stl.clear() ;
	stl.push_back(tr("Tunnel requests")) ;
	top_level_t_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_t_requests) ;

	top_level_hashes.clear() ;

	// load settings
    processSettings(true);
}

TurtleRouterDialog::~TurtleRouterDialog()
{
    // save settings
    processSettings(false);
}

void TurtleRouterDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("TurtleRouterDialog"));

    if (bLoad) {
        // load settings

        // state of splitter
        //splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        //Settings->setValue("Splitter", splitter->saveState());

    }

    Settings->endGroup();
    
    m_bProcessSettings = false;

}

bool sr_Compare( TurtleSearchRequestDisplayInfo m1,  TurtleSearchRequestDisplayInfo m2)  { return m1.age < m2.age; }

void TurtleRouterDialog::updateDisplay()
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleSearchRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleTunnelRequestDisplayInfo > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;
	
	std::sort(search_reqs_info.begin(),search_reqs_info.end(),sr_Compare) ;
	
	updateTunnelRequests(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

}

QString TurtleRouterDialog::getPeerName(const RsPeerId& peer_id)
{
    static std::map<RsPeerId, QString> names ;

    std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return "unknown peer";

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

void TurtleRouterDialog::updateTunnelRequests(	const std::vector<std::vector<std::string> >& hashes_info, 
																const std::vector<std::vector<std::string> >& tunnels_info, 
																const std::vector<TurtleSearchRequestDisplayInfo >& search_reqs_info,
																const std::vector<TurtleTunnelRequestDisplayInfo >& tunnel_reqs_info)
{
	// now display this in the QTableWidgets

	QStringList stl ;

	// remove all children of top level objects
	for(int i=0;i<_f2f_TW->topLevelItemCount();++i)
	{
		QTreeWidgetItem *taken ;
		while( (taken = _f2f_TW->topLevelItem(i)->takeChild(0)) != NULL) 
			delete taken ;
	}

	for(uint i=0;i<hashes_info.size();++i)
		findParentHashItem(hashes_info[i][0]) ;

	bool unknown_hash_found = false ;

	// check that an entry exist for all hashes
	for(uint i=0;i<tunnels_info.size();++i)
	{
		const std::string& hash(tunnels_info[i][3]) ;

		QTreeWidgetItem *parent = findParentHashItem(hash) ;

		if(parent->text(0).left(14) == tr("Unknown hashes"))
			unknown_hash_found = true ;
		
		float num = strtof(tunnels_info[i][5].c_str(), NULL);  // printFloatNumber
		char tmp[100] ;
		std::string units[4] = { "B/s","KB/s","MB/s","GB/s" } ;
		int k=0 ;
		while(num >= 800.0f && k<4)
			num /= 1024.0f,++k;
		sprintf(tmp,"%3.2f %s",num,units[k].c_str()) ;

		QString str = tr("Tunnel id") + ": " + QString::fromUtf8(tunnels_info[i][0].c_str()) + "\t" + tr("Speed") + ":  " + QString::fromStdString(tmp) + "\t " + tr("last transfer") + ": " + QString::fromStdString(tunnels_info[i][4])+ "\t" + QString::fromUtf8(tunnels_info[i][2].c_str()) + " -> " + QString::fromUtf8(tunnels_info[i][1].c_str());
		stl.clear() ;
		stl.push_back(str) ;
		QTreeWidgetItem *item = new QTreeWidgetItem(stl);
		parent->addChild(item);
		QFont font = item->font(0);
		if(strtol(tunnels_info[i][4].c_str(), NULL, 0)>10) // stuck
		{
			font.setItalic(true);
			item->setFont(0,font);
		}
		if(strtof(tunnels_info[i][5].c_str(), NULL)>1000) // fast
		{
			font.setBold(true);
			item->setFont(0,font);
		}
	}

	for(uint i=0;i<search_reqs_info.size();++i)
	{
		QString str = tr("Request id: %1\t %3 secs ago\t from  %2\t %4 (%5 hits)").arg(search_reqs_info[i].request_id,0,16).arg(getPeerName(search_reqs_info[i].source_peer_id), -25).arg(search_reqs_info[i].age).arg(QString::fromUtf8(search_reqs_info[i].keywords.c_str(),search_reqs_info[i].keywords.length())).arg(QString::number(search_reqs_info[i].hits));
		
		stl.clear() ;
		stl.push_back(str) ;

		top_level_s_requests->addChild(new QTreeWidgetItem(stl)) ;
	}
	top_level_s_requests->setText(0, tr("Search requests") + " (" + QString::number(search_reqs_info.size()) + ")" ) ;

	for(uint i=0;i<tunnel_reqs_info.size();++i)
		if(i+MAX_TUNNEL_REQUESTS_DISPLAY >= tunnel_reqs_info.size() || i < MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			QString str = tr("Request id: %1\t from [%2]\t %3 secs ago").arg(tunnel_reqs_info[i].request_id,0,16).arg(getPeerName(tunnel_reqs_info[i].source_peer_id)).arg(tunnel_reqs_info[i].age);

			stl.clear() ;
			stl.push_back(str) ;

			top_level_t_requests->addChild(new QTreeWidgetItem(stl)) ;
		}
		else if(i == MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			stl.clear() ;
			stl.push_back(QString("...")) ;
			top_level_t_requests->addChild(new QTreeWidgetItem(stl)) ;

		} 

	top_level_t_requests->setText(0, tr("Tunnel requests") + " ("+QString::number(tunnel_reqs_info.size()) + ")") ;

	QTreeWidgetItem *unknown_hashs_item = findParentHashItem(RsFileHash().toStdString()) ;
	unknown_hashs_item->setText(0,tr("Unknown hashes") + " (" + QString::number(unknown_hashs_item->childCount())+QString(")")) ;

	// Ok, this is a N2 search, but there are very few elements in the list.
	for(int i=2;i<_f2f_TW->topLevelItemCount();)
	{
		bool found = false ;

		if(_f2f_TW->topLevelItem(i)->text(0).left(14) == tr("Unknown hashes") && unknown_hash_found)
			found = true ;

		if(_f2f_TW->topLevelItem(i)->childCount() > 0)	// this saves uploading hashes
			found = true ;

		for(uint j=0;j<hashes_info.size() && !found;++j)
			if(_f2f_TW->topLevelItem(i)->text(0).toStdString() == hashes_info[j][0]) 
				found=true ;

		if(!found)
			delete _f2f_TW->takeTopLevelItem(i) ;
		else
			++i ;
	}
}
	
QTreeWidgetItem *TurtleRouterDialog::findParentHashItem(const std::string& hash)
{
	static const std::string null_hash = RsFileHash().toStdString() ;

	// look for the hash, and insert a new element if necessary.
	//
	QList<QTreeWidgetItem*> items = _f2f_TW->findItems((hash==null_hash)?tr("Unknown hashes"):QString::fromStdString(hash),Qt::MatchStartsWith) ;

	if(items.empty())
	{	
		QStringList stl ;
		stl.push_back((hash==null_hash)?tr("Unknown hashes"):QString::fromStdString(hash)) ;
		QTreeWidgetItem *item = new QTreeWidgetItem(_f2f_TW,stl) ;
		_f2f_TW->insertTopLevelItem(0,item) ;

		return item ;
	}
	else
		return items.front() ;
}

//=======================================================================================================================//

TunnelStatisticsDialog::TunnelStatisticsDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	m_bProcessSettings = false;

	maxWidth = 200 ;
	maxHeight = 200 ;

	// load settings
    processSettings(true);
}

TunnelStatisticsDialog::~TunnelStatisticsDialog()
{
    // save settings
    processSettings(false);
}

void TunnelStatisticsDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("TurtleRouterStatistics"));

    if (bLoad) {
        // load settings
    } else {
        // save settings
    }

    Settings->endGroup();
    
    m_bProcessSettings = false;
}

QString TunnelStatisticsDialog::getPeerName(const RsPeerId &peer_id)
{
    static std::map<RsPeerId, QString> names ;

    std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return tr("Unknown Peer");

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

QString TunnelStatisticsDialog::getPeerName(const RsGxsId& gxs_id)
{
    static std::map<RsGxsId, QString> names ;

    std::map<RsGxsId,QString>::const_iterator it = names.find(gxs_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsIdentityDetails detail ;

		if(!rsIdentity->getIdDetails(gxs_id,detail))
			return tr("Unknown Peer");

		return (names[gxs_id] = QString::fromUtf8(detail.mNickname.c_str())) ;
	}
}


QString TunnelStatisticsDialog::speedString(float f)
{
	if(f < 1.0f)
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void TunnelStatisticsDialog::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void TunnelStatisticsDialog::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();

    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

    QWidget::resizeEvent(event);
    update();
}
//=======================================================================================================================//

GxsAuthenticatedTunnelsDialog::GxsAuthenticatedTunnelsDialog(QWidget *parent)
    : TunnelStatisticsDialog(parent)
{
}

void GxsAuthenticatedTunnelsDialog::updateDisplay()
{
    // Request info about ongoing tunnels
    
    std::vector<RsGxsTunnelService::GxsTunnelInfo> tunnel_infos ;
    
    rsGxsTunnel->getTunnelsInfo(tunnel_infos) ;
    
 	//    // Tunnel information
 	//           
	//    GxsTunnelId  tunnel_id ;        // GXS Id we're talking to
	//    RsGxsId  destination_gxs_id ;   // GXS Id we're talking to
	//    RsGxsId  source_gxs_id ;	          // GXS Id we're using to talk
	//    uint32_t tunnel_status ;	          // active, requested, DH pending, etc.
	//    uint32_t total_size_sent ;	          // total bytes sent through that tunnel since openned (including management). 
	//    uint32_t total_size_received ;	  // total bytes received through that tunnel since openned (including management). 

	//    // Data packets

	//    uint32_t pending_data_packets;         // number of packets not acknowledged by other side, still on their way. Should be 0 unless something bad happens.
	//    uint32_t total_data_packets_sent ;     // total number of data packets sent (does not include tunnel management)
	//    uint32_t total_data_packets_received ; // total number of data packets received (does not include tunnel management)
        
    // now draw the shit
	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(Qt::transparent);
	//setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);

        // extracts the height of the fonts in pixels. This is used to calibrate the size of the objects to draw.

        float fontHeight = QFontMetricsF(font()).height();
        float fact = fontHeight/14.0;
	//maxHeight = 500*fact ;

	int cellx = 6*fact ;
	int celly = (10+4)*fact ;
    	int ox=5*fact,oy=5*fact ;

	painter.setPen(QColor::fromRgb(0,0,0)) ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Authenticated tunnels:")) ; oy += celly ;
        
    	for(uint32_t i=0;i<tunnel_infos.size();++i)
	{
		// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
		// draw...

		painter.drawText(ox+4*cellx,oy+celly,tr("Tunnel ID: %1").arg(QString::fromStdString(tunnel_infos[i].tunnel_id.toStdString()))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("from: %1 (%2)").arg(QString::fromStdString(tunnel_infos[i].source_gxs_id.toStdString())).arg(getPeerName(tunnel_infos[i].source_gxs_id))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("to: %1 (%2)").arg(QString::fromStdString(tunnel_infos[i].destination_gxs_id.toStdString())).arg(getPeerName(tunnel_infos[i].destination_gxs_id))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("status: %1").arg(QString::number(tunnel_infos[i].tunnel_status))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("total sent: %1 bytes").arg(QString::number(tunnel_infos[i].total_size_sent))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("total recv: %1 bytes").arg(QString::number(tunnel_infos[i].total_size_received))) ; oy += celly ;
        
		//	painter.drawLine(0,oy,maxWidth,oy) ;
		//	oy += celly ;
	}

	// update the pixmap
	//
	pixmap = tmppixmap;
	maxHeight = std::max(oy,10*celly);
}

//=======================================================================================================================//

GxsNetTunnelsDialog::GxsNetTunnelsDialog(QWidget *parent)
    : TunnelStatisticsDialog(parent)
{
}

static QString getGroupStatusString(RsGxsNetTunnelGroupInfo::GroupStatus group_status)
{
    switch(group_status)
    {
    default:
	   	    case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            : return QObject::tr("Unknown") ;
	   	    case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE               : return QObject::tr("Idle");
	   	    case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    : return QObject::tr("Virtual peers available");
    }
    return QString();
}

static QString getGroupPolicyString(RsGxsNetTunnelGroupInfo::GroupPolicy group_policy)
{
    switch(group_policy)
    {
    default:
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_UNKNOWN            : return QObject::tr("Unknown") ;
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE            : return QObject::tr("Passive") ;
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE             : return QObject::tr("Active") ;
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_REQUESTING         : return QObject::tr("Requesting peers") ;
    }
    return QString();
}

static QString getLastContactString(time_t last_contact)
{
    time_t now = time(NULL);

    if(last_contact == 0)
        return QObject::tr("Never");

    return QString::number(now - last_contact) + " secs ago" ;
}

static QString getServiceNameString(uint16_t service_id)
{
	static RsPeerServiceInfo ownServices;

    if(ownServices.mServiceList.find(service_id) == ownServices.mServiceList.end())
		rsServiceControl->getOwnServices(ownServices);

    return QString::fromUtf8(ownServices.mServiceList[service_id].mServiceName.c_str()) ;
}

void GxsNetTunnelsDialog::updateDisplay()
{
	// Request info about ongoing tunnels

	std::map<RsGxsGroupId,RsGxsNetTunnelGroupInfo> groups;                                 // groups on the client and server side
	std::map<RsGxsNetTunnelVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo> virtual_peers;    // current virtual peers, which group they provide, and how to talk to them through turtle
	Bias20Bytes bias;

	rsGxsDistSync->getStatistics(groups,virtual_peers,bias) ;

    // RsGxsNetTunnelGroupInfo:
    //
	//   enum GroupStatus {
	//   	    RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            = 0x00,	// unknown status
	//   	    RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE               = 0x01,	// no virtual peers requested, just waiting
	//   	    RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    = 0x02	// some virtual peers are available. Data can be read/written
	//   };
	//   enum GroupPolicy {
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_UNKNOWN            = 0x00,	// nothing has been set
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE            = 0x01,	// group is available for server side tunnels, but does not explicitely request tunnels
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE             = 0x02,	// group will only explicitely request tunnels if none available
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_REQUESTING         = 0x03,	// group explicitely requests tunnels
	//      };
    //
	//   RsGxsNetTunnelGroupInfo() : group_policy(RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE),group_status(RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE),last_contact(0) {}
    //
	//   GroupPolicy        group_policy ;
	//   GroupStatus        group_status ;
	//   time_t             last_contact ;
	//   RsFileHash         hash ;
	//   uint16_t           service_id ;
	//   std::set<RsPeerId> virtual_peers ;

    // now draw the shit
	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(Qt::transparent);
	//setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);

	// extracts the height of the fonts in pixels. This is used to calibrate the size of the objects to draw.

	float fontHeight = QFontMetricsF(font()).height();
	float fact = fontHeight/14.0;

	int cellx = 6*fact ;
	int celly = (10+4)*fact ;
	int ox=5*fact,oy=5*fact ;

	painter.setPen(QColor::fromRgb(0,0,0)) ;
	painter.drawText(ox+2*cellx,oy+celly,tr("GXS Groups:")) ; oy += celly ;

	for(auto it(groups.begin());it!=groups.end();++it)
	{
		// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
		// draw...

		painter.drawText(ox+4*cellx,oy+celly,tr("Service: %1 (%2) - Group ID: %3,\t policy: %4, \tstatus: %5, \tlast contact: %6, \t%7 virtual peers.")
                .arg(QString::number(it->second.service_id))
                .arg(getServiceNameString(it->second.service_id))
                .arg(QString::fromStdString(it->first.toStdString()))
                .arg(getGroupPolicyString(it->second.group_policy))
                .arg(getGroupStatusString(it->second.group_status))
                .arg(getLastContactString(it->second.last_contact))
                .arg(QString::number(it->second.virtual_peers.size()))
		                 ) ; oy += celly ;
	}

	painter.drawText(ox+2*cellx,oy+celly,tr("Virtual peers:")) ; oy += celly ;

	// update the pixmap
	//
	pixmap = tmppixmap;
	maxHeight = std::max(oy,10*celly);
}

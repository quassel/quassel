/** ***** QxtRPCPeer demonstration *****

This example shows the power of QxtRPCPeer with QxtSemaphore
It shows two QxtSpanSliders, both connected with the range property.
Run this app twice and watch the sliders sync
When QxtnamedPipe is ready, and the improvement to QxtRPCPeer is done, it will be posible to do the same 
without actualy blocking a port.

QxtSemaphore is experimental. Do not kill the app, use the guis close button instead
if you already killed the app, the semaphore might be blocked, try changing it if the app doesn't work.
*/

#include <QxtApplication>
#include <QxtRPCPeer>
#include <QxtSpanSlider>
#include <QxtSemaphore>

int main(int argc, char** argv)
	{
	QxtApplication app(argc,argv);
	QxtSpanSlider slider;
	QxtRPCPeer peer;
	peer.setRPCType(QxtRPCPeer::Peer);

	QxtSemaphore smp("org.libqxt.examples.rpcpeer.singleinstance");
	if(smp.trylock())
		peer.listen ( QHostAddress::LocalHost, 56663);	
	else
		peer.connect ( QHostAddress::LocalHost, 56663);

	peer.attachSignal (&slider, SIGNAL(spanChanged(int,int)));
	peer.attachSlot ( "spanChanged",&slider, SLOT(setSpan(int,int))); 

	slider.resize(80,200);
	slider.show();
	app.exec();
	};

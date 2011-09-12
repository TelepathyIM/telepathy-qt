<service xmlns="http://telepathy.freedesktop.org/wiki/service-profile-v1"
         id="test-profile"
         type="IM"
         provider="TestProfileProvider"
         manager="testprofilecm"
         protocol="testprofileproto"
         icon="test-profile-icon">
  <name>TestProfile</name>

  <parameters>
    <parameter name="server"  type="s" mandatory="1">profile.com</parameter>
    <parameter name="port"    type="u" mandatory="1">1111</parameter>
  </parameters>

  <presences allow-others="1">
    <presence id="available" label="Online"  icon="online" message="true"/>
    <presence id="offline"   label="Offline"/>
    <presence id="away"      label="Gone"    message="true"/>
    <presence id="xa"        label="Extended Away"/>
    <presence id="hidden"    disabled="1"/>
  </presences>

  <unsupported-channel-classes>
    <!-- this service doesn't support text chats and roomlists -->
    <channel-class>
      <property name="org.freedesktop.Telepathy.Channel.TargetHandleType"
                type="u">1</property>
      <property name="org.freedesktop.Telepathy.Channel.ChannelType"
                type="s">org.freedesktop.Telepathy.Channel.Type.Text</property>
    </channel-class>
    <channel-class>
      <property name="org.freedesktop.Telepathy.Channel.TargetHandleType"
                type="u">3</property>
      <property name="org.freedesktop.Telepathy.Channel.ChannelType"
                type="s">org.freedesktop.Telepathy.Channel.Type.Text</property>
    </channel-class>
  </unsupported-channel-classes>

</service>

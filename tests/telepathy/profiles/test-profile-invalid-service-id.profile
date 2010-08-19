<service id="another-test-profile"
         xmlns="http://telepathy.freedesktop.org/wiki/service-profile-v1">
  <type>IM</type>
  <provider>TestProfileProvider</provider>
  <name>TestProfile</name>
  <icon>test-profile-icon</icon>

  <manager>testprofilecm</manager>
  <protocol>testprofileproto</protocol>

  <parameters>
    <parameter name="server"  type="s" mandatory="1">profile.com</parameter>
    <parameter name="port"    type="u" mandatory="1">1111</parameter>
  </parameters>

  <presences allow-others="1">
    <presence id="available" label="Online"  icon="online"/>
    <presence id="offline"   label="Offline"/>
    <presence id="away"      label="Gone"/>
    <presence id="hidden"    disabled="1"/>
  </presences>

  <unsupported-channel-classes>
    <!-- this service doesn't support text roomlists -->
    <channel-class>
      <property name="org.freedesktop.Telepathy.Channel.TargetHandleType"
                type="u">3</property>
      <property name="org.freedesktop.Telepathy.Channel.ChannelType"
                type="s">org.freedesktop.Telepathy.Channel.Type.Text</property>
    </channel-class>
  </unsupported-channel-classes>

</service>

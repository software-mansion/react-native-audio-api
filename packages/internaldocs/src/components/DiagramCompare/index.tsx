import React from 'react';
import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

type Props = {
  originals: string[];
  children: React.ReactNode;
};

export default function DiagramCompare({ originals, children }: Props) {
  return (
    <Tabs groupId="diagram-format">
      <TabItem value="original" label="Original">
        {originals.map((src) => (
          <figure key={src}>
            <img src={src} alt="" />
          </figure>
        ))}
      </TabItem>
      <TabItem value="new" label="New">
        {children}
      </TabItem>
    </Tabs>
  );
}

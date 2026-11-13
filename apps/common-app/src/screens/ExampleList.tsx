import { useNavigation } from '@react-navigation/native';
import type { FC } from 'react';
import React from 'react';

import { ListRow, ListSection } from '../components';
import type { Example } from '../examples';
import TabScreen from './TabScreen';

export interface ExampleListSection {
  key: string;
  title?: string;
  color: string;
  examples: Example[];
}

interface ExampleListProps {
  title: string;
  sections: ExampleListSection[];
}

const ExampleList: FC<ExampleListProps> = ({ title, sections }) => {
  const navigation = useNavigation();

  return (
    <TabScreen title={title}>
      {sections.map((section) => (
        <ListSection key={section.key} title={section.title}>
          {section.examples.map((example) => (
            <ListRow
              key={example.key}
              title={example.title}
              icon={example.icon}
              iconColor={section.color}
              onPress={() => navigation.navigate(example.key)}
            />
          ))}
        </ListSection>
      ))}
    </TabScreen>
  );
};

export default ExampleList;
